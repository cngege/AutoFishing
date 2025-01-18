#include <fmt/format.h>
#include <functional>
#include <ll/api/Config.h>
#include <ll/api/command/Command.h>
#include <ll/api/command/CommandHandle.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/data/KeyValueDB.h>
#include <ll/api/event/EventBus.h>
#include <ll/api/event/ListenerBase.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/event/player/PlayerUseItemEvent.h>
#include <ll/api/form/ModalForm.h>
#include <ll/api/io/FileUtils.h>
#include <ll/api/mod/NativeMod.h>
#include <ll/api/mod/ModManagerRegistry.h>
#include <ll/api/service/Bedrock.h>
#include <mc/world/actor/ActorType.h>
#include <mc/server/commands/CommandOrigin.h>
#include <mc/server/commands/CommandOutput.h>
#include <mc/server/commands/CommandPermissionLevel.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/actor/FishingHook.h>
#include <mc/world/phys/HitResult.h>
#include <mc/world/level/Tick.h>
#include <memory>
#include <stdexcept>
#include "MyMod.h"

#include "ll/api/memory/Hook.h"
#include "ll/api/memory/Symbol.h"

int fishinghook_offset = -1;

int tickcount = 0;
std::unordered_map<Player*, bool> playerhash;

bool findFishinghook_offset() {
    auto updateServer= (ll::memory::resolveIdentifier(&FishingHook::_updateServer));
    // 往后寻找 能判断鱼是否上钩的偏移
    for (int i = 0; i < 200; i++) {
        if (*reinterpret_cast<std::byte*>((intptr_t)updateServer + i) == (std::byte)0x89
            && *reinterpret_cast<std::byte*>((intptr_t)updateServer + i + 1) == (std::byte)0x81) {
            fishinghook_offset = *reinterpret_cast<int*>((intptr_t)updateServer + i + 2);
            //my_mod::MyMod::getInstance().getSelf().getLogger().info("updateServer:{},i:{},fishinghook_offset:{}", updateServer,i,fishinghook_offset);
            return true;
        } else {
            i++;
        }
    }
    // 自动查找偏移失败 弹出提示 不进行下一步动作
    fishinghook_offset = -1;
    return false;
}

/// <summary>
/// 获取鱼钩中鱼的咬钩进度
/// </summary>
/// <param name="fishingHook">鱼钩单例</param>
/// <returns>0:未上钩</returns>
int GetHookedTime(FishingHook* fishingHook) {
    if (fishinghook_offset <= 0) {
        return 0;
    } else {
        return *reinterpret_cast<int*>((intptr_t)fishingHook + fishinghook_offset);
    }
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    Fishing_hitCheckHook,
    ll::memory::HookPriority::Normal,
    FishingHook,
    &FishingHook::_hitCheck,
    HitResult
) {
    // my_mod::MyMod::getInstance().getSelf().getLogger().info("A OK");
    //my_mod::MyMod::getInstance().getSelf().getLogger().info("A OK:{},time: {}", (void*)this, GetHookedTime(this));
    //return origin(a1);
    auto ret        = origin();
    int  HookedTime = GetHookedTime(this);
    //printf_s("0x%llX", (void*)thi);
    if (fishinghook_offset > 0 && HookedTime > 0) {
        auto actor = this->getOwner();
        if (actor->isPlayer()) {
            auto player = (Player*)actor;
            //thi->retrieve();        //直接调用这个函数的话会出BUG
            auto item = (ItemStack*)&(player->getSelectedItem());
            item->use(*player);
            if (this != nullptr) {
                this->remove();
            }
            player->refreshInventory();

            // 设置标志位 间隔 0.5 - 1 秒后再次抛竿
            if (tickcount > 10) tickcount = 0;
            playerhash[player] = true;
        }
    }
    return ret;
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    PlayertickWorldHook,
    ll::memory::HookPriority::Normal,
    Player,
    &Player::$tickWorld,
    void,
    Tick const& tick
) {
    if (fishinghook_offset <= 0) {
        return origin(tick);
    }
    if (tickcount < 20) {
        tickcount++;
    } else {
        if (playerhash[this]) {
            // 检测手中物品是否是鱼竿
            ItemStack* item = (ItemStack*)&(this->getSelectedItem());
            if (item->getTypeName() == "minecraft:fishing_rod") {
                item->use(*this);
            }
            playerhash[this] = false;
        }
        tickcount = 0;
    }
    origin(tick);
}

#include "mod/MyMod.h"

#include <memory>

#include "ll/api/mod/RegisterHelper.h"
#include <ll/api/memory/Memory.h>

bool findFishinghook_offset();

namespace my_mod {

MyMod& MyMod::getInstance() {
    static MyMod instance;
    return instance;
}

bool MyMod::load() {
    if (!findFishinghook_offset()) {
        my_mod::MyMod::getInstance().getSelf().getLogger().warn("没能找到关键偏移,插件自动停用,请反馈给开发者并等待更新");
        my_mod::MyMod::getInstance().getSelf().getLogger().warn("https://github.com/cngege/AutoFishing");
    }

    // Code for loading the plugin goes here.
    return true;
}

bool MyMod::enable() {
    return true;
}

bool MyMod::disable() {
    return true;
}

} // namespace my_mod

LL_REGISTER_MOD(my_mod::MyMod, my_mod::MyMod::getInstance());

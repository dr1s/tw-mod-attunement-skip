# tw-mod-attunement-skip

Automatically grants Vanilla raid attunement items by in-game mail and completes raid attunement quests when players reach level 60.

## Configuration

Copy `conf/tw-mod-attunement-skip.conf.dist` to your module config directory as `tw-mod-attunement-skip.conf` and adjust:

```ini
[AttunementSkip]
AttunementSkip.Enable = 1
AttunementSkip.DrakefireAmulet = 1
AttunementSkip.OnyxiaScaleCloak = 1
AttunementSkip.Naxx40Attunement = 1
AttunementSkip.MoltenCoreAttunement = 1
AttunementSkip.BlackwingLairAttunement = 1
```

- `Enable`: master toggle. Set to `0` to disable the module.
- `DrakefireAmulet`: grant [Drakefire Amulet] (item 16309) by mail at level 60.
- `OnyxiaScaleCloak`: grant [Onyxia Scale Cloak] (item 15138) by mail at level 60.
- `Naxx40Attunement`: auto-complete the Naxxramas 40 attunement quests (9121, 9122, 9123) at level 60.
- `MoltenCoreAttunement`: auto-complete [Attunement to the Core] (quest 7848) at level 60.
- `BlackwingLairAttunement`: auto-complete [Blackhand's Command] (quest 7761) at level 60.

## How it works

The module registers two scripts:

- A `WorldScript` loads the configuration when the world starts and whenever the config is reloaded.
- A `PlayerScript` processes attunements on `OnLogin` and `OnLevelChanged` for any player that has reached level 60.

Items are sent via mail so they are received even when the player has full bags. Each item is sent in a separate mail because Vanilla mail can only carry one item attachment. Items are only sent if the player does not already own them in their inventory, bank, or mail inbox. If an item is deleted, it will be mailed again on the next login. Quests are only completed if they have not already been rewarded.

## Build

```sh
cmake -S . -B build -DMODULES=static
cmake -S . -B build -DMODULES=dynamic
# or per-module:
cmake -S . -B build -DMODULE_TW_MOD_ATTUNEMENT_SKIP=static
```

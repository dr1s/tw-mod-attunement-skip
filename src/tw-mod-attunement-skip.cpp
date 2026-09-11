#include "ScriptObjects.h"
#include "Config/Config.h"
#include "Database/DatabaseEnv.h"
#include "Item.h"
#include "Mail.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Log.h"

#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr uint32 ITEM_DRAKEFIRE_AMULET   = 16309;
    constexpr uint32 ITEM_ONYXIA_SCALE_CLOAK = 15138;

    constexpr uint32 QUEST_NAXX40_ATTUNEMENT_1 = 9121; // The Archangel's Payload
    constexpr uint32 QUEST_NAXX40_ATTUNEMENT_2 = 9122; // Echoes of War
    constexpr uint32 QUEST_NAXX40_ATTUNEMENT_3 = 9123; // The Only Song I Know...
    constexpr uint32 QUEST_MOLTEN_CORE_ATTUNEMENT = 7848; // Attunement to the Core
    constexpr uint32 QUEST_BLACKWING_LAIR_ATTUNEMENT = 7761; // Blackhand's Command

    constexpr uint8 LEVEL_CLASSIC_RAIDS = 60;

    struct AttunementConfig
    {
        bool enabled = true;
        bool grantDrakefireAmulet = true;
        bool grantOnyxiaScaleCloak = true;
        bool completeNaxx40Attunement = true;
        bool completeMoltenCoreAttunement = true;
        bool completeBlackwingLairAttunement = true;
    };

    AttunementConfig s_config;

    bool PlayerHasItemInInventoryOrBank(Player* player, uint32 itemId)
    {
        return player && player->HasItemCount(itemId, 1, true);
    }

    bool PlayerHasItemInMail(Player* player, uint32 itemId)
    {
        if (!player)
            return false;

        QueryResult* result = CharacterDatabase.PQuery(
            "SELECT 1 FROM mail_items mi JOIN mail m ON mi.mail_id = m.id "
            "WHERE m.receiver = %u AND mi.item_template = %u LIMIT 1",
            player->GetGUIDLow(), itemId);

        bool const found = result != nullptr;
        delete result;
        return found;
    }

    bool PlayerHasItem(Player* player, uint32 itemId)
    {
        return PlayerHasItemInInventoryOrBank(player, itemId) || PlayerHasItemInMail(player, itemId);
    }

    void SendAttunementMails(Player* player, std::vector<std::pair<uint32, bool>> const& items)
    {
        if (!player)
            return;

        for (auto const& [itemId, enabled] : items)
        {
            if (!enabled)
                continue;

            if (PlayerHasItem(player, itemId))
                continue;

            ItemPrototype const* itemProto = sObjectMgr.GetItemPrototype(itemId);
            if (!itemProto)
                continue;

            std::string subject = "Your Attunement Item";
            std::string body = "Greetings,\n\nAs you have reached level " +
                               std::to_string(LEVEL_CLASSIC_RAIDS) +
                               ", you have been granted the following attunement item to access raid content:\n\n- " +
                               itemProto->Name1 +
                               "\n\nThis item is required for raid entry. Good luck!\n";

            Item* item = Item::CreateItem(itemId, 1, player);
            if (!item)
                continue;

            MailDraft(subject, body)
                .AddItem(item)
                .SendMailTo(MailReceiver(player), MailSender(MAIL_NORMAL, uint32(0), MAIL_STATIONERY_GM), MAIL_CHECK_MASK_HAS_BODY, 0);

            sLog.outString("[tw-mod-attunement-skip] sent attunement mail to player %s for item %s.", player->GetName(), itemProto->Name1.c_str());
        }
    }

    void AutoCompleteQuest(Player* player, uint32 questId)
    {
        if (!player || !player->IsInWorld())
            return;

        if (player->GetQuestRewardStatus(questId))
            return;

        Quest const* quest = sObjectMgr.GetQuestTemplate(questId);
        if (!quest)
            return;

        if (player->GetQuestStatus(questId) == QUEST_STATUS_NONE)
            player->AddQuest(quest, nullptr);

        if (!player->GetQuestRewardStatus(questId))
            player->RewardQuest(quest, 0, player, false);
    }

    void ProcessPlayerLevel(Player* player)
    {
        if (!s_config.enabled)
            return;

        if (!player || !player->IsInWorld())
            return;

        if (player->GetLevel() < LEVEL_CLASSIC_RAIDS)
            return;

        std::vector<std::pair<uint32, bool>> classicItems = {
            { ITEM_DRAKEFIRE_AMULET,   s_config.grantDrakefireAmulet },
            { ITEM_ONYXIA_SCALE_CLOAK, s_config.grantOnyxiaScaleCloak }
        };

        SendAttunementMails(player, classicItems);

        if (s_config.completeNaxx40Attunement)
        {
            AutoCompleteQuest(player, QUEST_NAXX40_ATTUNEMENT_1);
            AutoCompleteQuest(player, QUEST_NAXX40_ATTUNEMENT_2);
            AutoCompleteQuest(player, QUEST_NAXX40_ATTUNEMENT_3);
        }

        if (s_config.completeMoltenCoreAttunement)
            AutoCompleteQuest(player, QUEST_MOLTEN_CORE_ATTUNEMENT);

        if (s_config.completeBlackwingLairAttunement)
            AutoCompleteQuest(player, QUEST_BLACKWING_LAIR_ATTUNEMENT);
    }

    void LoadAttunementConfig()
    {
        s_config.enabled = sConfig.GetBoolDefault("AttunementSkip.Enable", true);
        s_config.grantDrakefireAmulet = sConfig.GetBoolDefault("AttunementSkip.DrakefireAmulet", true);
        s_config.grantOnyxiaScaleCloak = sConfig.GetBoolDefault("AttunementSkip.OnyxiaScaleCloak", true);
        s_config.completeNaxx40Attunement = sConfig.GetBoolDefault("AttunementSkip.Naxx40Attunement", true);
        s_config.completeMoltenCoreAttunement = sConfig.GetBoolDefault("AttunementSkip.MoltenCoreAttunement", true);
        s_config.completeBlackwingLairAttunement = sConfig.GetBoolDefault("AttunementSkip.BlackwingLairAttunement", true);
    }

    class TwModAttunementSkipWorldScript : public WorldScript
    {
    public:
        TwModAttunementSkipWorldScript()
            : WorldScript("tw-mod-attunement-skip_world", { WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED, WORLDHOOK_ON_AFTER_CONFIG_LOAD })
        {
        }

        void OnBeforeWorldInitialized() override
        {
            sLog.outString("[tw-mod-attunement-skip] module loaded.");
            LoadAttunementConfig();
        }

        void OnAfterConfigLoad(bool /*reload*/) override
        {
            LoadAttunementConfig();
        }
    };

    class TwModAttunementSkipPlayerScript : public PlayerScript
    {
    public:
        TwModAttunementSkipPlayerScript()
            : PlayerScript("tw-mod-attunement-skip_player", { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LEVEL_CHANGED })
        {
        }

        void OnLogin(Player* player) override
        {
            ProcessPlayerLevel(player);
        }

        void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override
        {
            ProcessPlayerLevel(player);
        }
    };
}

void Addtw_mod_attunement_skipScripts()
{
    new TwModAttunementSkipWorldScript();
    new TwModAttunementSkipPlayerScript();
}

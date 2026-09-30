/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_RANDOMPLAYERBOTMGR_H
#define PLAYERBOTS_RANDOMPLAYERBOTMGR_H

#include "GameTime.h"
#include "NewRpgInfo.h"
#include "ObjectGuid.h"
#include "PlayerbotCommandServer.h"
#include "PlayerbotMgr.h"
#include <unordered_set>

struct BattlegroundInfo
{
    std::vector<uint32> bgInstances;
    std::vector<uint32> ratedArenaInstances;
    std::vector<uint32> skirmishArenaInstances;
    uint32 bgInstanceCount = 0;
    uint32 ratedArenaInstanceCount = 0;
    uint32 skirmishArenaInstanceCount = 0;
    uint32 minLevel = 0;
    uint32 maxLevel = 0;
    uint32 activeRatedArenaQueue = 0;     // 0 = Inactive, 1 = Active
    uint32 activeSkirmishArenaQueue = 0;  // 0 = Inactive, 1 = Active
    uint32 activeBgQueue = 0;             // 0 = Inactive, 1 = Active

    // Bots (Arena)
    uint32 ratedArenaBotCount = 0;
    uint32 skirmishArenaBotCount = 0;

    // Bots (Battleground)
    uint32 bgHordeBotCount = 0;
    uint32 bgAllianceBotCount = 0;

    // Players (Arena)
    uint32 ratedArenaPlayerCount = 0;
    uint32 skirmishArenaPlayerCount = 0;

    // Players (Battleground)
    uint32 bgHordePlayerCount = 0;
    uint32 bgAlliancePlayerCount = 0;
};

class ChatHandler;
class PerfMonitorOperation;
class WorldLocation;

struct CachedEvent
{
    uint32 value = 0;
    uint32 lastChangeTime = 0;
    uint32 validIn = 0;
    std::string data;

    bool IsEmpty() const { return !lastChangeTime; }
};

struct BotEventCache
{
    bool loaded = false;
    std::unordered_map<std::string, CachedEvent> events;
};

// https://gist.github.com/bradley219/5373998

class botPIDImpl;
class botPID
{
public:
    // Kp -  proportional gain
    // Ki -  Integral gain
    // Kd -  derivative gain
    // dt -  loop interval time
    // max - maximum value of manipulated variable
    // min - minimum value of manipulated variable
    botPID(double dt, double max, double min, double Kp, double Ki, double Kd);
    void adjust(double Kp, double Ki, double Kd);
    void reset();

    double calculate(double setpoint, double pv);
    ~botPID();

private:
    botPIDImpl* pimpl;
};

class RandomPlayerbotMgr : public PlayerbotHolder
{
public:
    static RandomPlayerbotMgr& instance()
    {
        static RandomPlayerbotMgr instance;

        return instance;
    }

    void LogPlayerLocation();
    void UpdateAIInternal(uint32 elapsed, bool minimal = false) override;

    uint32 activeBots = 0;
    static bool HandlePlayerbotConsoleCommand(ChatHandler* handler, char const* args);
    // F2 — Overland travel parties. Form one party (Phase 1: Orgrimmar -> Wailing Caverns) that
    // marches on foot then disbands. Fired by the GM command and (in test mode) the coordinator timer.
    std::string SpawnTravelParty();  // returns leader name on success, "" on failure
    static bool HandleTravelPartyCommand(ChatHandler* handler, char const* args);
    bool IsInTravelParty(Player* bot);  // public: also used to force-activate marching bots
    bool IsRandomBot(Player* bot);
    bool IsRandomBot(ObjectGuid::LowType bot);
    bool IsAddclassBot(Player* bot);
    bool IsAddclassBot(ObjectGuid::LowType bot);
    void Randomize(Player* bot);
    void Clear(Player* bot);
    void RandomizeFirst(Player* bot);
    void RandomizeMin(Player* bot);
    void IncreaseLevel(Player* bot);
    void ScheduleTeleport(uint32 bot, uint32 time = 0);
    void ScheduleChangeStrategy(uint32 bot, uint32 time = 0);
    void HandleCommand(uint32 type, std::string const text, Player* fromPlayer, std::string channelName = "");
    std::string const HandleRemoteCommand(std::string const request);
    void OnPlayerLogout(Player* player);
    void OnPlayerLogin(Player* player);
    void OnPlayerLoginError(uint32 bot);
    Player* GetRandomPlayer();
    std::vector<Player*> GetPlayers() { return players; };
    PlayerBotMap GetAllBots() { return playerBots; };
    void InitArenaTeams();
    void PrintStats();
    double GetBuyMultiplier(Player* bot);
    double GetSellMultiplier(Player* bot);
    void AddTradeDiscount(Player* bot, Player* master, int32 value);
    void SetTradeDiscount(Player* bot, Player* master, uint32 value);
    uint32 GetTradeDiscount(Player* bot, Player* master);
    void Refresh(Player* bot);
    void RandomTeleportForLevel(Player* bot);
    void RandomTeleportGrindForLevel(Player* bot);
    void RandomTeleportForRpg(Player* bot);
    uint32 GetMaxAllowedBotCount();
    bool ProcessBot(Player* player);
    void Revive(Player* player);
    void ChangeStrategy(Player* player);
    void ChangeStrategyOnce(Player* player);
    uint32 GetValue(Player* bot, std::string const& type);
    uint32 GetValue(uint32 bot, std::string const& type);
    std::string GetData(uint32 bot, std::string const& type);
    void SetValue(uint32 bot, std::string const& type, uint32 value, std::string const& data = "");
    void SetValue(Player* bot, std::string const& type, uint32 value, std::string const& data = "");
    bool IsSpecPvp(uint32 bot, uint8 cls);
    void Remove(Player* bot);
    ObjectGuid GetBattleMasterGUID(Player* bot, BattlegroundTypeId bgTypeId);
    CreatureData const* GetCreatureDataByEntry(uint32 entry);
    void LoadBattleMastersCache();
    std::map<uint32, std::map<uint32, BattlegroundInfo>> BattlegroundData;
    std::map<uint32, std::map<uint32, std::map<TeamId, uint32>>> VisualBots;
    std::map<uint32, std::map<uint32, std::map<uint32, uint32>>> Supporters;
    std::map<TeamId, std::vector<uint32>> LfgDungeons;
    void CheckBgQueue();
    void CheckLfgQueue();
    void CheckPlayers();
    void LogBattlegroundInfo();

    std::map<TeamId, std::map<BattlegroundTypeId, std::vector<uint32>>> getBattleMastersCache()
    {
        return BattleMastersCache;
    }

    float getActivityMod() { return activityMod; }
    float getActivityPercentage() { return activityMod * 100.0f; }
    void setActivityPercentage(float percentage) { activityMod = percentage / 100.0f; }
    static uint8 GetTeamClassIdx(bool isAlliance, uint8 claz) { return isAlliance * 20 + claz; }

    void PrepareAddclassCache();
    void Init();
    std::map<uint8, std::unordered_set<ObjectGuid>> addclassCache;

    // Account type management
    void AssignAccountTypes();
    bool IsAccountType(uint32 accountId, uint8 accountType);

protected:
    void OnBotLoginInternal(Player* const bot) override;

private:
    RandomPlayerbotMgr() : PlayerbotHolder()
    {
        this->playersLevel = sPlayerbotAIConfig.randombotStartingLevel;

        if (sPlayerbotAIConfig.enabled || sPlayerbotAIConfig.randomBotAutologin)
        {
            PlayerbotCommandServer::instance().Start();
        }

        BattlegroundData.clear();  // Clear here and here only.

        // Cleanup on server start: orphaned pet data that's often left behind by bot pets that no longer exist in the DB
        CharacterDatabase.Execute("DELETE FROM pet_aura WHERE guid NOT IN (SELECT id FROM character_pet)");
        CharacterDatabase.Execute("DELETE FROM pet_spell WHERE guid NOT IN (SELECT id FROM character_pet)");
        CharacterDatabase.Execute("DELETE FROM pet_spell_cooldown WHERE guid NOT IN (SELECT id FROM character_pet)");

        for (int bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
        {
            for (int queueType = BATTLEGROUND_QUEUE_AV; queueType < MAX_BATTLEGROUND_QUEUE_TYPES; ++queueType)
            {
                this->BattlegroundData[queueType][bracket] = BattlegroundInfo();
            }
        }

        this->BgCheckTimer = 0;
        this->LfgCheckTimer = 0;
        this->PlayersCheckTimer = 0;
    }

    ~RandomPlayerbotMgr() = default;

    RandomPlayerbotMgr(RandomPlayerbotMgr const&) = delete;
    RandomPlayerbotMgr& operator=(RandomPlayerbotMgr const&) = delete;

    RandomPlayerbotMgr(RandomPlayerbotMgr&&) = delete;
    RandomPlayerbotMgr& operator=(RandomPlayerbotMgr&&) = delete;

    // pid values are set in constructor
    botPID pid = botPID(1, 50, -50, 0, 0, 0);
    float activityMod = 0.25;
    bool _isBotInitializing = true;
    bool _isBotLogging = true;
    NewRpgStatistic rpgStasticTotal;
    CachedEvent* FindEvent(uint32 bot, std::string const& event);
    uint32 GetEventValue(uint32 bot, std::string const& event);
    std::string GetEventData(uint32 bot, std::string const& event);
    uint32 SetEventValue(uint32 bot, std::string const& event, uint32 value, uint32 validIn,
                         std::string const& data = "");
    void GetBots();
    std::vector<uint32> GetBgBots(uint32 bracket);
    time_t BgCheckTimer;
    time_t LfgCheckTimer;
    time_t PlayersCheckTimer;
    time_t RealPlayerLastTimeSeen = 0;
    time_t DelayLoginBotsTimer;
    time_t printStatsTimer;
    uint32 AddRandomBots();
    bool ProcessBot(uint32 bot);
    void ScheduleRandomize(uint32 bot, uint32 time);
    void RandomTeleport(Player* bot);
    void RandomTeleport(Player* bot, std::vector<WorldLocation>& locs, bool hearth = false);
    uint32 GetZoneLevel(uint16 mapId, float teleX, float teleY, float teleZ);
    std::vector<WorldLocation> GetPlayerZoneTeleportLocations(std::vector<WorldLocation> const& locs, Player* bot);
    typedef void (RandomPlayerbotMgr::*ConsoleCommandHandler)(Player*);
    std::vector<Player*> players;

    // Populate-Around-Players (F1)
    struct ZoneDemand
    {
        uint32 zoneId = 0;
        uint32 target = 0;
        float allianceShare = 0.5f;
        bool allowAlliance = true;
        bool allowHorde = true;
        uint32 playersAlliance = 0;
        uint32 playersHorde = 0;
        uint32 minPlayerLevel = 0;
        uint32 maxPlayerLevel = 0;
        uint32 curAlliance = 0;
        uint32 curHorde = 0;
        uint32 curPeer = 0;
        uint32 curTexture = 0;
    };
    std::unordered_map<uint32, ZoneDemand> _playerZones;
    uint32 _playerZoneRefreshTime = 0;
    void RefreshPlayerZones();
    bool IsPopulatePeer(ZoneDemand const& demand, uint32 level) const;
    std::vector<WorldLocation> GetPlayerZoneTeleportLocations(Player* bot);

    // F2 — Overland travel parties
    struct TravelParty
    {
        ObjectGuid leaderGuid;
        std::vector<ObjectGuid> memberGuids;  // includes the leader
        TravelDestination* dest = nullptr;    // self-owned; freed on disband
        WorldPosition* destPos = nullptr;     // self-owned; freed on disband
        uint32 mapId = 0;
        float destX = 0.f, destY = 0.f, destZ = 0.f;
        uint32 deadline = 0;    // unix time; disband when exceeded (crude anti-stuck watchdog)
        uint32 lastLogTime = 0; // throttle for the progress log during the live walk test
        float maxMemberDist = 0.f;  // farthest follower from leader (crew cohesion)
        std::vector<WorldLocation> waypoints;  // current navmesh partial path toward the entrance
        size_t wpIndex = 0;                    // next waypoint the leader is walking toward
        // Smooth movement: aim MovePoint at the FAR end of the path for one long spline, and only
        // re-issue when near it / stopped (re-issuing every tick resets the spline = choppy).
        float curTgtX = 0.f, curTgtY = 0.f, curTgtZ = 0.f;
        bool curTgtSet = false;
        // Descend-to-door: the entrance sits in a ravine, so the navmesh path from the RIM to the
        // door poly is only partial and the leader circles the rim. Track the best 3D distance to
        // the real door; when it plateaus near the entrance, switch to a straight descent.
        float bestDoorDist = 1e9f;
        uint32 bestProgressTime = 0;  // last time bestDoorDist improved (init to spawn time)
        bool descending = false;
        // Multi-stage route (outdoor cave MOUTH -> inside PORTAL). destX/Y/Z is the CURRENT stage's
        // target; advance on arrival, disband after the last. General for Phase 2 approach chains.
        std::vector<WorldLocation> route;
        size_t routeIdx = 0;
    };
    std::vector<TravelParty> _travelParties;
    uint32 _travelPartyTime = 0;
    void UpdateTravelParties();               // per-tick: check arrival/timeout, disband
    void DisbandTravelParty(TravelParty& p);  // tear down group + AI, free owned dest

    // std::map<uint32, std::vector<WorldLocation>> rpgLocsCache;
    std::map<uint32, std::map<uint32, std::vector<WorldLocation>>> rpgLocsCacheLevel;
    std::map<TeamId, std::map<BattlegroundTypeId, std::vector<uint32>>> BattleMastersCache;
    std::unordered_map<uint32, BotEventCache> eventCache;
    std::unordered_set<uint32> currentBots;
    uint32 playersLevel;

    // Account lists
    std::vector<uint32> rndBotTypeAccounts;             // Accounts marked as RNDbot (type 1)
    std::vector<uint32> addClassTypeAccounts;           // Accounts marked as AddClass (type 2)

    //void ScaleBotActivity();      // Deprecated function
    static inline uint32 NowSeconds() { return static_cast<uint32>(GameTime::GetGameTime().count()); }
};

#define sRandomPlayerbotMgr RandomPlayerbotMgr::instance()

#endif

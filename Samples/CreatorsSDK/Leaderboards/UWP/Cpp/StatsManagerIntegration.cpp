// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"
#include <time.h>
#include "Leaderboards.h"

using namespace Concurrency;
using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation;
using namespace xbox::services;
using namespace xbox::services::stats::manager;

void Sample::InitializeStatsManager()
{
    m_statsManager = stats_manager::get_singleton_instance();
}

void Sample::AddUserToStatsManager(_In_ std::shared_ptr<xbox::services::system::xbox_live_user> user)
{
    stringstream_t source;
    source << _T("Adding user ");
    source << user->gamertag();
    source << _T(" to StatsManager");
    m_console->WriteLine(source.str().c_str());

    m_statsManager->add_local_user(user);
}

void Sample::RemoveUserFromStatsManager(_In_ std::shared_ptr<xbox::services::system::xbox_live_user> user)
{
    m_statsManager->remove_local_user(user);
}

void Sample::SetStatForUser(
    _In_ std::shared_ptr<xbox::services::system::xbox_live_user> user,
    _In_ const string_t& statName,
    _In_ int64_t statValue)
{
    m_statsManager->set_stat_as_integer(user, statName, statValue);

    // Typically stats will be uploaded automatically
    // you should only request to flush when a game session or level ends.
    m_statsManager->request_flush_to_service(user, false);

    stringstream_t source;
    source << _T("Setting ");
    source << statName;
    source << _T(" to ");
    source << statValue;
    m_console->WriteLine(source.str().c_str());
}

void Sample::GetLeaderboard(
    _In_ std::shared_ptr<xbox::services::system::xbox_live_user> user, 
    _In_ const string_t& statName
    )
{
    leaderboard::leaderboard_query leaderboardQuery;
    m_statsManager->get_leaderboard(user, statName, leaderboardQuery);

    stringstream_t source;
    source << _T("Getting leaderboard for ");
    source << statName;
    m_console->WriteLine(source.str().c_str());
}

void Sample::GetLeaderboardSkipToRank(
    _In_ std::shared_ptr<xbox::services::system::xbox_live_user> user,
    _In_ const string_t& statName
    )
{
    leaderboard::leaderboard_query leaderboardQuery;
    leaderboardQuery.set_skip_result_to_rank(10);
    m_statsManager->get_leaderboard(user, statName, leaderboardQuery);

    stringstream_t source;
    source << _T("Getting skip to rank leaderboard for ");
    source << statName;
    m_console->WriteLine(source.str().c_str());
}

void Sample::GetLeaderboardSkipToSelf(
    _In_ std::shared_ptr<xbox::services::system::xbox_live_user> user,
    _In_ const string_t& statName
    )
{
    leaderboard::leaderboard_query leaderboardQuery;
    leaderboardQuery.set_skip_result_to_me(true);
    m_statsManager->get_leaderboard(user, statName, leaderboardQuery);

    stringstream_t source;
    source << _T("Getting skip to self leaderboard for ");
    source << statName;
    m_console->WriteLine(source.str().c_str());
}

void Sample::GetLeaderboardForSocialGroup(
    _In_ std::shared_ptr<xbox::services::system::xbox_live_user> user, 
    _In_ const string_t& statName,
    _In_ const string_t& socialGroup
    )
{
    leaderboard::leaderboard_query leaderboardQuery;
    // Social leaderboards can optionally set the sort order via:
    //leaderboardQuery.set_order(sortOrder);
    m_statsManager->get_social_leaderboard(user, statName, socialGroup, leaderboardQuery);

    stringstream_t source;
    source << _T("Getting social leaderboard for ");
    source << statName;
    m_console->WriteLine(source.str().c_str());
}

void Sample::UpdateStatsManager()
{
    // Process events from the stats manager
    // This should be called each frame update

    auto statsEvents = m_statsManager->do_work();
    std::wstring text;

    for (const auto& evt : statsEvents)
    {
        switch (evt.event_type())
        {
            case stat_event_type::local_user_added: 
                text = L"local_user_added"; 
                break;

            case stat_event_type::local_user_removed: 
                text = L"local_user_removed"; 
                break;

            case stat_event_type::stat_update_complete: 
                text = L"stat_update_complete"; 
                break;

            case stat_event_type::get_leaderboard_complete: 
                text = L"get_leaderboard_complete";
                auto getLeaderboardCompleteArgs = std::dynamic_pointer_cast<leaderboard_result_event_args>(evt.event_args());
                ProcessLeaderboards(evt.local_user(), getLeaderboardCompleteArgs->result());
                break;
        }

        stringstream_t source;
        source << _T("StatsManager event: ");
        source << text;
        source << _T(".");
        m_console->WriteLine(source.str().c_str());
    }
}

void Sample::ProcessLeaderboards(
    _In_ std::shared_ptr<xbox::services::system::xbox_live_user> user,
    _In_ xbox::services::xbox_live_result<xbox::services::leaderboard::leaderboard_result> result
    )
{
    if (!result.err())
    {
        auto leaderboardResult = result.payload();
        PrintLeaderboard(leaderboardResult);

        // Keep processing if there is more data in the leaderboard
        if (leaderboardResult.has_next())
        {
            if (!leaderboardResult.get_next_query().err())
            {               
                auto lbQuery = leaderboardResult.get_next_query().payload();
                if (lbQuery.social_group().empty())
                {
                    m_statsManager->get_leaderboard(user, lbQuery.stat_name(), lbQuery);
                }
                else
                {
                    m_statsManager->get_social_leaderboard(user, lbQuery.stat_name(), lbQuery.social_group(), lbQuery);
                }
            }
        }
    }
}



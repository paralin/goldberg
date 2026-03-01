/* Copyright (C) 2019 Mr Goldberg
   This file is part of the Goldberg Emulator

   The Goldberg Emulator is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   The Goldberg Emulator is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the Goldberg Emulator; if not, see
   <http://www.gnu.org/licenses/>.  */

#include "dll/steam_gameserver_items.h"

void Steam_GameServer_Items::steam_gameserver_items_network_callback(void *object, Common_Message *msg)
{
    //PRINT_DEBUG_ENTRY();

    auto inst = (Steam_GameServer_Items *)object;
    inst->network_callback(msg);
}

void Steam_GameServer_Items::steam_gameserver_items_run_every_runcb(void *object)
{
    //PRINT_DEBUG_ENTRY();

    auto inst = (Steam_GameServer_Items *)object;
    inst->run_callbacks();
}

Steam_GameServer_Items::Steam_GameServer_Items(class Settings *settings, class Networking *network, class SteamCallBacks *callbacks, class SteamCallResults *callback_results, class RunEveryRunCB *run_every_runcb)
{
    this->settings = settings;
    this->network = network;
    this->callbacks = callbacks;
    this->callback_results = callback_results;
    this->run_every_runcb = run_every_runcb;

    this->network->setCallback(CALLBACK_ID_GAMESERVER_ITEMS, settings->get_local_steam_id(), &Steam_GameServer_Items::steam_gameserver_items_network_callback, this);
    this->network->setCallback(CALLBACK_ID_USER_STATUS, settings->get_local_steam_id(), &Steam_GameServer_Items::steam_gameserver_items_network_callback, this);
    this->run_every_runcb->add(Steam_GameServer_Items::steam_gameserver_items_run_every_runcb, this);
}

Steam_GameServer_Items::~Steam_GameServer_Items()
{
    this->network->rmCallback(CALLBACK_ID_GAMESERVER_ITEMS, settings->get_local_steam_id(), &Steam_GameServer_Items::steam_gameserver_items_network_callback, this);
    this->network->rmCallback(CALLBACK_ID_USER_STATUS, settings->get_local_steam_id(), &Steam_GameServer_Items::steam_gameserver_items_network_callback, this);
    this->run_every_runcb->remove(Steam_GameServer_Items::steam_gameserver_items_run_every_runcb, this);
}

SteamAPICall_t Steam_GameServer_Items::LoadItems( CSteamID ownerID )
{
    PRINT_DEBUG("%llu", ownerID.ConvertToUint64());
    std::lock_guard<std::recursive_mutex> lock(global_mutex);

    // see if we already have their inventory cached
    if (all_user_items.count(ownerID)) {
        const auto &items = all_user_items.at(ownerID);

        GSItemCount_t data{};
        data.m_OwnerID = ownerID;
        data.m_eResult = k_EItemRequestResultOK;
        data.m_unCount = static_cast<uint32>(items.size());
        SteamAPICall_t ret = callback_results->addCallResult(data.k_iCallback, &data, sizeof(data), 0.1);
        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data), 0.1);
        return ret;
    }

    // request inventory from this player
    RequestInventory new_request{};
    new_request.created = std::chrono::high_resolution_clock::now();
    new_request.steam_id = ownerID;
    new_request.steam_api_call = callback_results->reserveCallResult();
    pending_items_requests.push_back(new_request);

    auto request_msg = new GameServer_Items_Messages::InventoryRequest();
    request_msg->set_steam_api_call(new_request.steam_api_call);

    auto gameserver_items_msg = new GameServer_Items_Messages();
    gameserver_items_msg->set_type(GameServer_Items_Messages::Request_Inventory);
    gameserver_items_msg->set_allocated_inventory_request(request_msg);

    Common_Message msg{};
    msg.set_allocated_gameserver_items_messages(gameserver_items_msg);
    msg.set_source_id(settings->get_local_steam_id().ConvertToUint64());
    msg.set_dest_id(ownerID.ConvertToUint64());
    network->sendTo(&msg, true);

    return new_request.steam_api_call;
}

void Steam_GameServer_Items::LoadItems_old( CSteamID ownerID )
{
    PRINT_DEBUG_ENTRY();
    LoadItems(ownerID);
}

SteamAPICall_t Steam_GameServer_Items::GetItemCount( CSteamID ownerID )
{
    PRINT_DEBUG_ENTRY();
    return LoadItems(ownerID);
}

void Steam_GameServer_Items::GetItemCount_old( CSteamID ownerID )
{
    PRINT_DEBUG_ENTRY();
    GetItemCount(ownerID);
}

bool Steam_GameServer_Items::GetItemIterative( CSteamID ownerID, uint32 iIndex, uint64 *pulItemID, uint32 *punItemDefIndex, uint32 *punItemLevel, EItemQuality *peQuality, uint32 *punInventoryPos, uint32 *punQuantity, uint32 *punAttributeCount )
{
    PRINT_DEBUG("%u", iIndex);
    std::lock_guard<std::recursive_mutex> lock(global_mutex);

    if (!all_user_items.count(ownerID))
        return false;

    const auto &items = all_user_items.at(ownerID);
    if (iIndex >= items.size())
        return false;

    const Econ_Item &item = items[iIndex];
    *pulItemID = item.id;
    *punItemDefIndex = item.def;
    *punItemLevel = item.level;
    *peQuality = item.quality;
    *punInventoryPos = item.inv_pos;
    *punQuantity = item.quantity;
    *punAttributeCount = static_cast<uint32>(item.attributes.size());

    return true;
}

bool Steam_GameServer_Items::GetItemIterative( CSteamID ownerID, uint32 iIndex, uint64 *pulItemID, uint32 *punItemDefIndex, uint32 *punItemLevel, EItemQuality *peQuality, uint32 *punInventoryPos, uint32 *punAttributeCount )
{
    PRINT_DEBUG_ENTRY();
    uint32 quantity;
    return GetItemIterative(ownerID, iIndex, pulItemID, punItemDefIndex, punItemLevel, peQuality, punInventoryPos, &quantity, punAttributeCount);
}

bool Steam_GameServer_Items::GetItemByID( uint64 ulItemID, CSteamID *pOwnerID, uint32 *punItemDefIndex, uint32 *punItemLevel, EItemQuality *peQuality, uint32 *punInventoryPos, uint32 *punQuantity, uint32 *punAttributeCount )
{
    PRINT_DEBUG("%llu", ulItemID);
    std::lock_guard<std::recursive_mutex> lock(global_mutex);

    for (const auto &[steam_id, items] : all_user_items) {
        for (const Econ_Item &item : items) {
            if (item.id != ulItemID)
                continue;

            *pOwnerID = steam_id;
            *punItemDefIndex = item.def;
            *punItemLevel = item.level;
            *peQuality = item.quality;
            *punInventoryPos = item.inv_pos;
            *punQuantity = item.quantity;
            *punAttributeCount = static_cast<uint32>(item.attributes.size());

            return true;
        }
    }

    return false;
}

bool Steam_GameServer_Items::GetItemByID( uint64 ulItemID, uint32 *punItemDefIndex, uint32 *punItemLevel, EItemQuality *peQuality, uint32 *punInventoryPos, uint32 *punAttributeCount )
{
    PRINT_DEBUG_ENTRY();
    CSteamID steam_id;
    uint32 quantity;
    return GetItemByID(ulItemID, &steam_id, punItemDefIndex, punItemLevel, peQuality, punInventoryPos, &quantity, punAttributeCount);
}

bool Steam_GameServer_Items::GetItemAttribute( uint64 ulItemID, uint32 unAttributeIndex, uint32 *punAttributeDefIndex, float *pflAttributeValue )
{
    PRINT_DEBUG("%llu %u", ulItemID, unAttributeIndex);
    std::lock_guard<std::recursive_mutex> lock(global_mutex);

    for (const auto &[steamID, items] : all_user_items) {
        for (const Econ_Item &item : items) {
            if (item.id != ulItemID)
                continue;

            if (unAttributeIndex >= item.attributes.size())
                return false;

            *punAttributeDefIndex = item.attributes[unAttributeIndex].def;
            *pflAttributeValue = item.attributes[unAttributeIndex].value;

            return true;
        }
    }

    return false;
}

HNewItemRequest Steam_GameServer_Items::CreateNewItemRequest( CSteamID steamID )
{
    PRINT_DEBUG_TODO();
    return 0;
}

HNewItemRequest Steam_GameServer_Items::CreateNewItemRequest( CSteamID steamID, uint32 unItemLevel, EItemQuality eQuality )
{
    PRINT_DEBUG_TODO();
    return 0;
}

bool Steam_GameServer_Items::AddNewItemLevel( HNewItemRequest handle, uint32 unItemLevel )
{
    PRINT_DEBUG_TODO();
    return false;
}

bool Steam_GameServer_Items::AddNewItemQuality( HNewItemRequest handle, EItemQuality eQuality )
{
    PRINT_DEBUG_TODO();
    return false;
}

bool Steam_GameServer_Items::SetNewItemInitialInventoryPos( HNewItemRequest handle, uint32 unInventoryPos )
{
    PRINT_DEBUG_TODO();
    return false;
}

bool Steam_GameServer_Items::SetNewItemInitialQuantity( HNewItemRequest handle, uint32 unQuantity )
{
    PRINT_DEBUG_TODO();
    return false;
}

bool Steam_GameServer_Items::AddNewItemCriteria( HNewItemRequest handle, const char *pchField, EItemCriteriaOperator eOperator, const char *pchValue, bool bRequired )
{
    PRINT_DEBUG_TODO();
    return false;
}

bool Steam_GameServer_Items::AddNewItemCriteria( HNewItemRequest handle, const char *pchField, EItemCriteriaOperator eOperator, float flValue, bool bRequired )
{
    PRINT_DEBUG_TODO();
    return false;
}

SteamAPICall_t Steam_GameServer_Items::SendNewItemRequest( HNewItemRequest handle )
{
    PRINT_DEBUG_TODO();
    return k_uAPICallInvalid;
}

void Steam_GameServer_Items::SendNewItemRequest_old( HNewItemRequest handle )
{
    PRINT_DEBUG_ENTRY();
    SendNewItemRequest(handle);
}

SteamAPICall_t Steam_GameServer_Items::GrantItemToUser( uint64 ulItemID, CSteamID steamIDRecipient )
{
    PRINT_DEBUG_TODO();
    return k_uAPICallInvalid;
}

void Steam_GameServer_Items::GrantItemToUser_old( uint64 ulItemID, CSteamID steamIDRecipient )
{
    PRINT_DEBUG_ENTRY();
    GrantItemToUser(ulItemID, steamIDRecipient);
}

SteamAPICall_t Steam_GameServer_Items::DeleteTemporaryItem( uint64 ulItemID )
{
    PRINT_DEBUG_TODO();
    return k_uAPICallInvalid;
}

void Steam_GameServer_Items::DeleteTemporaryItem_old( uint64 ulItemID )
{
    PRINT_DEBUG_ENTRY();
    DeleteTemporaryItem(ulItemID);
}

SteamAPICall_t Steam_GameServer_Items::DeleteAllTemporaryItems()
{
    PRINT_DEBUG_TODO();
    return k_uAPICallInvalid;
}

void Steam_GameServer_Items::DeleteAllTemporaryItems_old()
{
    PRINT_DEBUG_ENTRY();
    DeleteAllTemporaryItems();
}

SteamAPICall_t Steam_GameServer_Items::UpdateQuantity( uint64 ulItemID, uint32 unNewQuantity )
{
    PRINT_DEBUG_TODO();
    return k_uAPICallInvalid;
}

SteamAPICall_t Steam_GameServer_Items::GetItemBlob( uint64 ulItemID )
{
    PRINT_DEBUG_TODO();
    return k_uAPICallInvalid;
}

SteamAPICall_t Steam_GameServer_Items::SetItemBlob( uint64 ulItemID, const void *pubBlob, uint32 cubBlob )
{
    PRINT_DEBUG_TODO();
    return k_uAPICallInvalid;
}

// user sent their inventory
void Steam_GameServer_Items::network_callback_inventory_response(Common_Message *msg)
{
    uint64 user_steamid = msg->source_id();

    PRINT_DEBUG("player sent their inventory %llu", user_steamid);
    if (!msg->gameserver_items_messages().has_inventory_response()) {
        PRINT_DEBUG("error empty msg");
        return;
    }

    const auto &response_msg = msg->gameserver_items_messages().inventory_response();

    // find this pending request
    auto it = std::find_if(
        pending_items_requests.begin(), pending_items_requests.end(),
        [=](const RequestInventory &item) {
            return item.steam_api_call == response_msg.steam_api_call() &&
                item.steam_id == user_steamid;
        }
    );
    if (pending_items_requests.end() == it) { // timeout and already removed
        PRINT_DEBUG("error got player inventory but pending request timedout/removed (doesn't exist)");
        return;
    }

    auto &items = all_user_items[user_steamid];
    items.clear();

    for (const auto &item : response_msg.items()) {
        Econ_Item new_item;
        new_item.id = item.id();
        new_item.def = item.def();
        new_item.level = item.level();
        new_item.quality = static_cast<EItemQuality>(item.quality());
        new_item.inv_pos = item.inv_pos();
        new_item.quantity = item.quantity();
        if (new_item.id == 0)
            continue;

        for (const auto &attr : item.attributes()) {
            Econ_Item_Attribute new_attr;
            new_attr.def = attr.def();
            new_attr.value = attr.value();
            if (new_attr.def == 0)
                continue;

            new_item.attributes.push_back(new_attr);
        }

        items.push_back(new_item);
    }

    GSItemCount_t data{};
    data.m_OwnerID = user_steamid;
    data.m_eResult = k_EItemRequestResultOK;
    data.m_unCount = static_cast<uint32>(items.size());
    callback_results->addCallResult(it->steam_api_call, data.k_iCallback, &data, sizeof(data));
    callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));

    // remove this pending request
    pending_items_requests.erase(it);

    PRINT_DEBUG("server got player inventory: %u items", response_msg.items_size());
}

// user updated item inventory position
void Steam_GameServer_Items::network_callback_inventory_pos_update(Common_Message *msg)
{
    uint64 user_steamid = msg->source_id();

    PRINT_DEBUG("player updated item inventory position %llu", user_steamid);
    if (!msg->gameserver_items_messages().has_inventory_pos_update()) {
        PRINT_DEBUG("error empty msg");
        return;
    }

    if (!all_user_items.count(user_steamid)) {
        PRINT_DEBUG("error no inventory for player", user_steamid);
        return;
    }

    const auto &inventory_msg = msg->gameserver_items_messages().inventory_pos_update();
    uint64 item_id = inventory_msg.item_id();
    uint32 item_inv_pos = inventory_msg.item_inv_pos();

    auto &items = all_user_items.at(user_steamid);

    for (Econ_Item &item : items) {
        if (item.id != item_id)
            continue;

        item.inv_pos = item_inv_pos;

        GSItemInventoryPosUpdated_t data{};
        data.m_SteamID = user_steamid;
        data.m_ulItemID = item_id;
        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data), 0.15);

        PRINT_DEBUG("server got updated item inventory position: %llu 0x%08x", item_id, item_inv_pos);
        return;
    }

    PRINT_DEBUG("error item %llu not found", item_id);
}

// user deleted an item
void Steam_GameServer_Items::network_callback_item_deletion(Common_Message *msg)
{
    uint64 user_steamid = msg->source_id();

    PRINT_DEBUG("player deleted inventory item %llu", user_steamid);
    if (!msg->gameserver_items_messages().has_item_deletion()) {
        PRINT_DEBUG("error empty msg");
        return;
    }

    if (!all_user_items.count(user_steamid)) {
        PRINT_DEBUG("error no inventory for player", user_steamid);
        return;
    }

    const auto &drop_msg = msg->gameserver_items_messages().item_deletion();
    uint64 item_id = drop_msg.item_id();

    auto &items = all_user_items.at(user_steamid);

    for (auto it = items.begin(); it != items.end(); it++) {
        if (it->id != item_id)
            continue;

        items.erase(it);

        GSItemDeleted_t data{};
        data.m_SteamID = user_steamid;
        data.m_ulItemID = item_id;
        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data), 0.15);

        PRINT_DEBUG("server deleted inventory item: %llu", item_id);
        return;
    }

    PRINT_DEBUG("error item %llu not found", item_id);
}

// only triggered when we have a message
void Steam_GameServer_Items::network_callback(Common_Message *msg)
{
    // this should never happen, but just in case
    if (msg->source_id() == settings->get_local_steam_id().ConvertToUint64()) return;

    if (msg->has_gameserver_items_messages()) {
        switch (msg->gameserver_items_messages().type()) {
        // user sent their inventory
        case GameServer_Items_Messages::Response_Inventory:
            network_callback_inventory_response(msg);
        break;

        // user updated item inventory position
        case GameServer_Items_Messages::Request_UpdateInventoryPos:
            network_callback_inventory_pos_update(msg);
        break;

        // user deleted an item
        case GameServer_Items_Messages::Request_DeleteItem:
            network_callback_item_deletion(msg);
        break;

        default:
            PRINT_DEBUG("unhandled type %i", (int)msg->gameserver_items_messages().type());
        break;
        }
    } else if (msg->has_low_level()) {
        if (msg->low_level().type() == Low_Level::DISCONNECT) {
            uint64 user_steamid = msg->source_id();
            all_user_items.erase(user_steamid);
        }
    }
}

void Steam_GameServer_Items::run_callbacks()
{
    for (auto it = pending_items_requests.begin(); it != pending_items_requests.end();) {
        if (check_timedout(it->created, 7.0)) {
            GSItemCount_t data{};
            data.m_OwnerID = it->steam_id;
            data.m_eResult = k_EItemRequestResultTimeout;
            data.m_unCount = 0;
            callback_results->addCallResult(it->steam_api_call, data.k_iCallback, &data, sizeof(data));
            callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));

            PRINT_DEBUG("player inventory request timeout %llu", it->steam_id);
            it = pending_items_requests.erase(it);
        } else {
            it++;
        }
    }
}

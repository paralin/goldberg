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

#include "dll/steam_user_items.h"

void Steam_User_Items::steam_user_items_network_callback(void *object, Common_Message *msg)
{
    //PRINT_DEBUG_ENTRY();

    auto inst = (Steam_User_Items *)object;
    inst->network_callback(msg);
}

void Steam_User_Items::load_items_from_file()
{
    items.clear();

    nlohmann::json items_json;
    if (!local_storage->load_json_file("", items_user_file, items_json))
        return;

    for (auto it = items_json.begin(); it != items_json.end(); it++) {
        Econ_Item new_item{};
        try {
            new_item.id = std::stoull(it.key());
        } catch (...) {
            continue;
        }
        if (new_item.id == 0)
            continue;

        new_item.def = it->value("definition", 0u); // 0 is a valid item definition
        new_item.level = it->value("level", 1u);
        new_item.quality = static_cast<EItemQuality>(it->value("quality", 0));
        new_item.inv_pos = it->value("inventory_pos", 0u);
        new_item.quantity = it->value("quantity", 0u); // TODO: not sure what this is for, TF2 does not seem to use it

        if (it->contains("attributes")) {
            for (const auto &attr : it->at("attributes")) {
                Econ_Item_Attribute new_attr;
                new_attr.def = attr.value("definition", 0u);
                new_attr.value = attr.value("value", 0.0f);
                if (new_attr.def == 0) // 0 is not a valid attribute definition, however
                    continue;

                new_item.attributes.push_back(new_attr);
            }
        }

        // add SteamID to item ID to avoid ID collisions in multiplayer games
        uint32 account_id = settings->get_local_steam_id().GetAccountID();

        if (settings->use_32bit_inventory_item_ids) {
            // 32-bit mode
            new_item.id <<= 20ull;
            new_item.id |= static_cast<uint64>(account_id) & 0x000FFFFFull;
        } else {
            // 64-bit mode
            new_item.id <<= 32ull;
            new_item.id |= static_cast<uint64>(account_id);
        }

        items.push_back(new_item);
    }
}

void Steam_User_Items::save_items_to_file()
{
    nlohmann::json items_json;

    for (const Econ_Item &item : items) {
        uint64 item_id = 0;
        if (settings->use_32bit_inventory_item_ids) {
            // 32-bit mode
            item_id = item.id >> 20ull;
        } else {
            // 64-bit mode
            item_id = item.id >> 32ull;
        }

        nlohmann::json json_item;
        json_item["definition"] = item.def;
        json_item["level"] = item.level;
        json_item["quality"] = item.quality;
        json_item["inventory_pos"] = item.inv_pos;
        json_item["quantity"] = item.quantity;

        for (const Econ_Item_Attribute &attr : item.attributes) {
            nlohmann::json json_attr;
            json_attr["definition"] = attr.def;
            json_attr["value"] = attr.value;
            json_item["attributes"].push_back(json_attr);
        }

        items_json[std::to_string(item_id)] = json_item;
    }

    local_storage->write_json_file("", items_user_file, items_json);
}

Steam_User_Items::Steam_User_Items(class Settings *settings, class Networking *network, class Local_Storage *local_storage, class SteamCallBacks *callbacks, class SteamCallResults *callback_results)
{
    this->settings = settings;
    this->network = network;
    this->local_storage = local_storage;
    this->callbacks = callbacks;
    this->callback_results = callback_results;

    this->network->setCallback(CALLBACK_ID_GAMESERVER_ITEMS, settings->get_local_steam_id(), &Steam_User_Items::steam_user_items_network_callback, this);
}

Steam_User_Items::~Steam_User_Items()
{
    this->network->rmCallback(CALLBACK_ID_GAMESERVER_ITEMS, settings->get_local_steam_id(), &Steam_User_Items::steam_user_items_network_callback, this);
}

SteamAPICall_t Steam_User_Items::LoadItems()
{
    PRINT_DEBUG_ENTRY();
    std::lock_guard<std::recursive_mutex> lock(global_mutex);

    if (!items_loaded) {
        load_items_from_file();
        items_loaded = true;
    }

    UserItemCount_t data{};
    data.m_gameID = settings->get_local_game_id();
    data.m_eResult = k_EItemRequestResultOK;
    data.m_unCount = static_cast<uint32>(items.size());
    SteamAPICall_t ret = callback_results->addCallResult(data.k_iCallback, &data, sizeof(data), 0.1);
    callbacks->addCBResult(data.k_iCallback, &data, sizeof(data), 0.1);
    return ret;
}

void Steam_User_Items::LoadItems_old()
{
    PRINT_DEBUG_ENTRY();
    LoadItems();
}

SteamAPICall_t Steam_User_Items::GetItemCount()
{
    PRINT_DEBUG_ENTRY();
    return LoadItems();
}

void Steam_User_Items::GetItemCount_old()
{
    PRINT_DEBUG_ENTRY();
    GetItemCount();
}

bool Steam_User_Items::GetItemIterative( uint32 iIndex, uint64 *pulItemID, uint32 *punItemDefIndex, uint32 *punItemLevel, EItemQuality *peQuality, uint32 *punInventoryPos, uint32 *punQuantity, uint32 *punAttributeCount )
{
    PRINT_DEBUG("%u", iIndex);
    std::lock_guard<std::recursive_mutex> lock(global_mutex);

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

bool Steam_User_Items::GetItemIterative( uint32 iIndex, uint64 *pulItemID, uint32 *punItemDefIndex, uint32 *punItemLevel, EItemQuality *peQuality, uint32 *punInventoryPos, uint32 *punAttributeCount )
{
    PRINT_DEBUG_ENTRY();
    uint32 quantity;
    return GetItemIterative(iIndex, pulItemID, punItemDefIndex, punItemLevel, peQuality, punInventoryPos, &quantity, punAttributeCount);
}

bool Steam_User_Items::GetItemByID( uint64 ulItemID, uint32 *punItemDefIndex, uint32 *punItemLevel, EItemQuality *peQuality, uint32 *punInventoryPos, uint32 *punQuantity, uint32 *punAttributeCount )
{
    PRINT_DEBUG("%llu", ulItemID);
    std::lock_guard<std::recursive_mutex> lock(global_mutex);

    for (const Econ_Item &item : items) {
        if (item.id != ulItemID)
            continue;

        *punItemDefIndex = item.def;
        *punItemLevel = item.level;
        *peQuality = item.quality;
        *punInventoryPos = item.inv_pos;
        *punQuantity = item.quantity;
        *punAttributeCount = static_cast<uint32>(item.attributes.size());

        return true;
    }

    return false;
}

bool Steam_User_Items::GetItemByID( uint64 ulItemID, uint32 *punItemDefIndex, uint32 *punItemLevel, EItemQuality *peQuality, uint32 *punInventoryPos, uint32 *punAttributeCount )
{
    PRINT_DEBUG_ENTRY();
    uint32 quantity;
    return GetItemByID(ulItemID, punItemDefIndex, punItemLevel, peQuality, punInventoryPos, &quantity, punAttributeCount);
}

bool Steam_User_Items::GetItemAttribute( uint64 ulItemID, uint32 unAttributeIndex, uint32 *punAttributeDefIndex, float *pflAttributeValue )
{
    PRINT_DEBUG("%llu %u", ulItemID, unAttributeIndex);
    std::lock_guard<std::recursive_mutex> lock(global_mutex);

    for (const Econ_Item &item : items) {
        if (item.id != ulItemID)
            continue;

        if (unAttributeIndex >= item.attributes.size())
            return false;

        *punAttributeDefIndex = item.attributes[unAttributeIndex].def;
        *pflAttributeValue = item.attributes[unAttributeIndex].value;

        return true;
    }

    return false;
}

//===============================================================================================================
// POSITION HANDLING
//===============================================================================================================
// TF Inventory Position cracking

// REALLY OLD FORMAT (??):
//      Bits 17-32 are the bag index (class index + 999, 1 is unequipped).
//      Bits 1-16 are the position of the item within the bag.
//      0 means item hasn't been acknowledged by the player yet.
//
// LESS OLD FORMAT (up through July, 2011):
//		If Bit 31 is 0: 
//			Bits 1-16 are the backpack position.
//			Bits 17-26 are a bool for whether the item is equipped in the matching class.
//		Otherwise, if Bit 31 is 1:
//			Item hasn't been acknowledged by the player yet.
//			Bits 1-16 are the method by the player found the item (see unacknowledged_item_inventory_positions_t)
//		Bit 32 is 1, to note the new format.
//
// CURRENT FORMAT:
//		If Bit 31 is 0: 
//			Bits 1-16 are the backpack position.
//		Otherwise, if Bit 31 is 1:
//			Item hasn't been acknowledged by the player yet.
//			Bits 1-16 are the method by the player found the item (see unacknowledged_item_inventory_positions_t)
//		Equipped state is stored elsewhere.
//		This is the only format that should exist on clients.
// Note (1/15/2013) For backwards compatibility, if the value is 0 item is considered unacknowledged too

SteamAPICall_t Steam_User_Items::UpdateInventoryPos( uint64 ulItemID, uint32 unNewInventoryPos )
{
    PRINT_DEBUG("%llu 0x%08x", ulItemID, unNewInventoryPos);
    std::lock_guard<std::recursive_mutex> lock(global_mutex);

    for (auto &item : items) {
        if (item.id != ulItemID)
            continue;

        item.inv_pos = unNewInventoryPos;
        save_items_to_file();

        // let the server know, too
        auto inventory_msg = new GameServer_Items_Messages::InventoryPosUpdate();
        inventory_msg->set_item_id(ulItemID);
        inventory_msg->set_item_inv_pos(unNewInventoryPos);

        auto gameserver_items_msg = new GameServer_Items_Messages();
        gameserver_items_msg->set_type(GameServer_Items_Messages::Request_UpdateInventoryPos);
        gameserver_items_msg->set_allocated_inventory_pos_update(inventory_msg);

        Common_Message msg{};
        msg.set_allocated_gameserver_items_messages(gameserver_items_msg);
        msg.set_source_id(settings->get_local_steam_id().ConvertToUint64());
        network->sendToAllGameservers(&msg, true);

        UpdateInventoryPosResponse_t data{};
        data.m_ulItemID = item.id;
        data.m_eResult = k_EItemRequestResultOK;
        SteamAPICall_t ret = callback_results->addCallResult(data.k_iCallback, &data, sizeof(data), 0.1);
        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data), 0.1);
        return ret;
    }

    UpdateInventoryPosResponse_t data{};
    data.m_ulItemID = ulItemID;
    data.m_eResult = k_EItemRequestResultNoMatch;
    SteamAPICall_t ret = callback_results->addCallResult(data.k_iCallback, &data, sizeof(data), 0.1);
    callbacks->addCBResult(data.k_iCallback, &data, sizeof(data), 0.1);
    return ret;
}

void Steam_User_Items::UpdateInventoryPos_old( uint64 ulItemID, uint32 unNewInventoryPos )
{
    PRINT_DEBUG_ENTRY();
    UpdateInventoryPos(ulItemID, unNewInventoryPos);
}

SteamAPICall_t Steam_User_Items::DeleteItem( uint64 ulItemID )
{
    PRINT_DEBUG("%llu", ulItemID);
    std::lock_guard<std::recursive_mutex> lock(global_mutex);

    for (auto it = items.begin(); it != items.end(); it++) {
        if (it->id != ulItemID)
            continue;

        items.erase(it);
        save_items_to_file();

        // let the server know, too
        auto drop_msg = new GameServer_Items_Messages::ItemDeletion();
        drop_msg->set_item_id(ulItemID);

        auto gameserver_items_msg = new GameServer_Items_Messages();
        gameserver_items_msg->set_type(GameServer_Items_Messages::Request_DeleteItem);
        gameserver_items_msg->set_allocated_item_deletion(drop_msg);

        Common_Message msg{};
        msg.set_allocated_gameserver_items_messages(gameserver_items_msg);
        msg.set_source_id(settings->get_local_steam_id().ConvertToUint64());
        network->sendToAllGameservers(&msg, true);

        DeleteItemResponse_t data{};
        data.m_ulItemID = ulItemID;
        data.m_eResult = k_EItemRequestResultOK;
        SteamAPICall_t ret = callback_results->addCallResult(data.k_iCallback, &data, sizeof(data), 0.1);
        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data), 0.1);
        return ret;
    }

    DeleteItemResponse_t data{};
    data.m_ulItemID = ulItemID;
    data.m_eResult = k_EItemRequestResultNoMatch;
    SteamAPICall_t ret = callback_results->addCallResult(data.k_iCallback, &data, sizeof(data), 0.1);
    callbacks->addCBResult(data.k_iCallback, &data, sizeof(data), 0.1);
    return ret;
}

void Steam_User_Items::DropItem_old( uint64 ulItemID )
{
    PRINT_DEBUG_ENTRY();
    DeleteItem(ulItemID);
}

SteamAPICall_t Steam_User_Items::DropItem_old2( uint64 ulItemID )
{
    PRINT_DEBUG_ENTRY();
    return DeleteItem(ulItemID);
}

SteamAPICall_t Steam_User_Items::GetItemBlob( uint64 ulItemID )
{
    PRINT_DEBUG_TODO();
    return k_uAPICallInvalid;
}

SteamAPICall_t Steam_User_Items::SetItemBlob( uint64 ulItemID, const void *pubBlob, uint32 cubBlob )
{
    PRINT_DEBUG_TODO();
    return k_uAPICallInvalid;
}

SteamAPICall_t Steam_User_Items::DropItem( uint64 ulItemID )
{
    PRINT_DEBUG_TODO();
    return k_uAPICallInvalid;
}

// server requested our inventory
void Steam_User_Items::network_callback_inventory_request(Common_Message *msg)
{
    if (!msg->gameserver_items_messages().has_inventory_request()) {
        PRINT_DEBUG("error empty msg");
        return;
    }

    uint64 server_steamid = msg->source_id();
    const auto &request_msg = msg->gameserver_items_messages().inventory_request();

    auto response_msg = new GameServer_Items_Messages::InventoryResponse();
    response_msg->set_steam_api_call(request_msg.steam_api_call());

    for (const Econ_Item &item : items) {
        auto new_item = response_msg->add_items();
        new_item->set_id(item.id);
        new_item->set_def(item.def);
        new_item->set_level(item.level);
        new_item->set_quality(static_cast<int32>(item.quality));
        new_item->set_inv_pos(item.inv_pos);
        new_item->set_quantity(item.quantity);

        for (const auto &attr : item.attributes) {
            auto new_attr = new_item->add_attributes();
            new_attr->set_def(attr.def);
            new_attr->set_value(attr.value);
        }
    }

    auto gameserver_items_msg = new GameServer_Items_Messages();
    gameserver_items_msg->set_type(GameServer_Items_Messages::Response_Inventory);
    gameserver_items_msg->set_allocated_inventory_response(response_msg);

    Common_Message new_msg{};
    new_msg.set_allocated_gameserver_items_messages(gameserver_items_msg);
    new_msg.set_source_id(settings->get_local_steam_id().ConvertToUint64());
    new_msg.set_dest_id(server_steamid);
    network->sendTo(&new_msg, true);

    PRINT_DEBUG("server requested inventory, sent %u items", items.size());
}

// only triggered when we have a message
void Steam_User_Items::network_callback(Common_Message *msg)
{
    switch (msg->gameserver_items_messages().type()) {
    // server requested our inventory
    case GameServer_Items_Messages::Request_Inventory:
        network_callback_inventory_request(msg);
    break;

    default:
        PRINT_DEBUG("unhandled type %i", (int)msg->gameserver_items_messages().type());
    break;
    }
}

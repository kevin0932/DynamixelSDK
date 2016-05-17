/*
* GroupBulkWrite.c
*
*  Created on: 2016. 5. 4.
*      Author: leon
*/
#if defined(_WIN32) || defined(_WIN64)
#define WINDLLEXPORT
#endif

#include <stdlib.h>
#include "dynamixel_sdk/GroupBulkWrite.h"

#define NOT_USED_ID         255

typedef struct
{
    UINT8_T     id_;
    UINT16_T    data_end_;
    UINT16_T    start_address_;
    UINT16_T    data_length_;
    UINT8_T     *data_;
}DataListBulkWrite;

typedef struct
{
    int         port_num;
    int         protocol_version;

    int         data_list_length_;

    bool        is_param_changed_;

    UINT16_T    param_length_;

    DataListBulkWrite   *data_list_;
}GroupDataBulkWrite;

GroupDataBulkWrite *groupDataBulkWrite;

int used_group_num_bulkwrite_ = 0;

int GroupBulkWrite_Size(int group_num)
{
    int _data_num;
    int size = 0;

    for (_data_num = 0; _data_num < groupDataBulkWrite[group_num].data_list_length_; _data_num++)
        if (groupDataBulkWrite[group_num].data_list_[_data_num].id_ != NOT_USED_ID)
            size++;
    return size;
}

int GroupBulkWrite_Find(int group_num, int id)
{
    int _data_num;

    for (_data_num = 0; _data_num < groupDataBulkWrite[group_num].data_list_length_; _data_num++)
        if (groupDataBulkWrite[group_num].data_list_[_data_num].id_ == id)
            break;

    return _data_num;
}

int GroupBulkWrite(int port_num, int protocol_version)
{
    int _group_num = 0;

    if (used_group_num_bulkwrite_ != 0)
        for (_group_num = 0; _group_num < used_group_num_bulkwrite_; _group_num++)
            if (groupDataBulkWrite[_group_num].is_param_changed_ != true)
                break;

    if (_group_num == used_group_num_bulkwrite_)
    {
        used_group_num_bulkwrite_++;
        groupDataBulkWrite = (GroupDataBulkWrite *)realloc(groupDataBulkWrite, used_group_num_bulkwrite_ * sizeof(GroupDataBulkWrite));
    }

    groupDataBulkWrite[_group_num].port_num = port_num;
    groupDataBulkWrite[_group_num].protocol_version = protocol_version;
    groupDataBulkWrite[_group_num].data_list_length_ = 0;
    groupDataBulkWrite[_group_num].is_param_changed_ = false;
    groupDataBulkWrite[_group_num].param_length_ = 0;
    groupDataBulkWrite[_group_num].data_list_ = 0;

    GroupBulkWrite_ClearParam(_group_num);

    return _group_num;
}

void GroupBulkWrite_MakeParam(int group_num)
{
    int _data_num, _idx, _c;
    int _port_num = groupDataBulkWrite[group_num].port_num;

    if (groupDataBulkWrite[group_num].protocol_version == 1)
        return;

    if (GroupBulkWrite_Size(group_num) == 0)
        return;

    if (packetData[_port_num].data_write_ != 0)
        free(packetData[_port_num].data_write_);
    packetData[_port_num].data_write_ = 0;

    groupDataBulkWrite[group_num].param_length_ = 0;

    _idx = 0;
    for (_data_num = 0; _data_num < groupDataBulkWrite[group_num].data_list_length_; _data_num++)
    {
        if (groupDataBulkWrite[group_num].data_list_[_data_num].id_ == NOT_USED_ID)
            continue;

        groupDataBulkWrite[group_num].param_length_ += 1 + 2 + 2 + groupDataBulkWrite[group_num].data_list_[_data_num].data_length_;

        packetData[_port_num].data_write_ = (UINT8_T*)realloc(packetData[_port_num].data_write_, groupDataBulkWrite[group_num].param_length_ * sizeof(UINT8_T)); 

        packetData[_port_num].data_write_[_idx++] = groupDataBulkWrite[group_num].data_list_[_data_num].id_;
        packetData[_port_num].data_write_[_idx++] = DXL_LOBYTE(groupDataBulkWrite[group_num].data_list_[_data_num].start_address_);
        packetData[_port_num].data_write_[_idx++] = DXL_HIBYTE(groupDataBulkWrite[group_num].data_list_[_data_num].start_address_);
        packetData[_port_num].data_write_[_idx++] = DXL_LOBYTE(groupDataBulkWrite[group_num].data_list_[_data_num].data_length_);
        packetData[_port_num].data_write_[_idx++] = DXL_HIBYTE(groupDataBulkWrite[group_num].data_list_[_data_num].data_length_);

        for (_c = 0; _c < groupDataBulkWrite[group_num].data_list_[_data_num].data_length_; _c++)
            packetData[_port_num].data_write_[_idx++] = groupDataBulkWrite[group_num].data_list_[_data_num].data_[_c];
    }
}

bool GroupBulkWrite_AddParam(int group_num, UINT8_T id, UINT16_T start_address, UINT16_T data_length, UINT32_T data, UINT16_T input_length)
{
    int _data_num = 0;

    if (groupDataBulkWrite[group_num].protocol_version == 1)
        return false;

    if (id == NOT_USED_ID)
        return false;

    if (groupDataBulkWrite[group_num].data_list_length_ != 0)
        _data_num = GroupBulkWrite_Find(group_num, id);

    if (groupDataBulkWrite[group_num].data_list_length_ == _data_num)
    {
        groupDataBulkWrite[group_num].data_list_length_++;
        groupDataBulkWrite[group_num].data_list_ = (DataListBulkWrite *)realloc(groupDataBulkWrite[group_num].data_list_, groupDataBulkWrite[group_num].data_list_length_ * sizeof(DataListBulkWrite));

        groupDataBulkWrite[group_num].data_list_[_data_num].id_ = id;
        groupDataBulkWrite[group_num].data_list_[_data_num].data_length_ = data_length;
        groupDataBulkWrite[group_num].data_list_[_data_num].start_address_ = start_address;
        groupDataBulkWrite[group_num].data_list_[_data_num].data_ = (UINT8_T *)calloc(groupDataBulkWrite[group_num].data_list_[_data_num].data_length_, sizeof(UINT8_T));
        groupDataBulkWrite[group_num].data_list_[_data_num].data_end_ = 0;
    }
    else
        if (groupDataBulkWrite[group_num].data_list_[_data_num].data_end_ + input_length > groupDataBulkWrite[group_num].data_list_[_data_num].data_length_)
            return false;

    switch (input_length)
    {
    case 1:
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[groupDataBulkWrite[group_num].data_list_[_data_num].data_end_ + 0] = DXL_LOBYTE(DXL_LOWORD(data));
        break;

    case 2:
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[groupDataBulkWrite[group_num].data_list_[_data_num].data_end_ + 0] = DXL_LOBYTE(DXL_LOWORD(data));
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[groupDataBulkWrite[group_num].data_list_[_data_num].data_end_ + 1] = DXL_HIBYTE(DXL_LOWORD(data));
        break;

    case 4:
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[groupDataBulkWrite[group_num].data_list_[_data_num].data_end_ + 0] = DXL_LOBYTE(DXL_LOWORD(data));
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[groupDataBulkWrite[group_num].data_list_[_data_num].data_end_ + 1] = DXL_HIBYTE(DXL_LOWORD(data));
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[groupDataBulkWrite[group_num].data_list_[_data_num].data_end_ + 2] = DXL_LOBYTE(DXL_HIWORD(data));
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[groupDataBulkWrite[group_num].data_list_[_data_num].data_end_ + 3] = DXL_HIBYTE(DXL_HIWORD(data));
        break;

    default:
        return false;
    }
    groupDataBulkWrite[group_num].data_list_[_data_num].data_end_ = input_length;

    groupDataBulkWrite[group_num].is_param_changed_ = true;
    return true;
}
void GroupBulkWrite_RemoveParam(int group_num, UINT8_T id)
{
    int _data_num = GroupBulkWrite_Find(group_num, id);

    if (groupDataBulkWrite[group_num].protocol_version == 1)
        return;

    if (_data_num == groupDataBulkWrite[group_num].data_list_length_)
        return;

    if (groupDataBulkWrite[group_num].data_list_[_data_num].id_ == NOT_USED_ID)  // NOT exist
        return;

    groupDataBulkWrite[group_num].data_list_[_data_num].data_end_ = 0;

    groupDataBulkWrite[group_num].data_list_[_data_num].data_ = 0;
    free(groupDataBulkWrite[group_num].data_list_[_data_num].data_);

    groupDataBulkWrite[group_num].data_list_[_data_num].data_length_ = 0;
    groupDataBulkWrite[group_num].data_list_[_data_num].start_address_ = 0;
    groupDataBulkWrite[group_num].data_list_[_data_num].id_ = NOT_USED_ID;

    groupDataBulkWrite[group_num].is_param_changed_ = true;
}

bool GroupBulkWrite_ChangeParam(int group_num, UINT8_T id, UINT16_T start_address, UINT16_T data_length, UINT32_T data, UINT16_T input_length, UINT16_T data_pos)
{
    int _data_num = GroupBulkWrite_Find(group_num, id);

    if (groupDataBulkWrite[group_num].protocol_version == 1)
        return false;

    if (id == NOT_USED_ID)
        return false;

    if (_data_num == groupDataBulkWrite[group_num].data_list_length_)
        return false;

    if (data_pos + input_length > groupDataBulkWrite[group_num].data_list_[_data_num].data_length_)
        return false;

    groupDataBulkWrite[group_num].data_list_[_data_num].data_length_ = data_length;
    groupDataBulkWrite[group_num].data_list_[_data_num].start_address_ = start_address;

    switch (input_length)
    {
    case 1:
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[data_pos + 0] = DXL_LOBYTE(DXL_LOWORD(data));
        break;

    case 2:
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[data_pos + 0] = DXL_LOBYTE(DXL_LOWORD(data));
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[data_pos + 1] = DXL_HIBYTE(DXL_LOWORD(data));
        break;

    case 4:
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[data_pos + 0] = DXL_LOBYTE(DXL_LOWORD(data));
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[data_pos + 1] = DXL_HIBYTE(DXL_LOWORD(data));
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[data_pos + 2] = DXL_LOBYTE(DXL_HIWORD(data));
        groupDataBulkWrite[group_num].data_list_[_data_num].data_[data_pos + 3] = DXL_HIBYTE(DXL_HIWORD(data));
        break;

    default:
        return false;
    }

    groupDataBulkWrite[group_num].is_param_changed_ = true;
    return true;
}
void GroupBulkWrite_ClearParam(int group_num)
{
    int _port_num = groupDataBulkWrite[group_num].port_num;

    if (groupDataBulkWrite[group_num].protocol_version == 1)
        return;

    if (GroupBulkWrite_Size(group_num) == 0)
        return;

    groupDataBulkWrite[group_num].data_list_ = 0;
    free(groupDataBulkWrite[group_num].data_list_);

    packetData[_port_num].data_write_ = 0;
    free(packetData[_port_num].data_write_);

    groupDataBulkWrite[group_num].data_list_length_ = 0;
}
void GroupBulkWrite_TxPacket(int group_num)
{
    if(groupDataBulkWrite[group_num].protocol_version == 1)
    {
        packetData[groupDataBulkWrite[group_num].port_num].communication_result_ =  COMM_NOT_AVAILABLE;
        return;
    }

    if (GroupBulkWrite_Size(group_num) == 0)
    {
        packetData[groupDataBulkWrite[group_num].port_num].communication_result_ = COMM_NOT_AVAILABLE;
        return;
    }

    if(groupDataBulkWrite[group_num].is_param_changed_ == true)
        GroupBulkWrite_MakeParam(group_num);

    BulkWriteTxOnly(groupDataBulkWrite[group_num].port_num, groupDataBulkWrite[group_num].protocol_version, groupDataBulkWrite[group_num].param_length_);
}

/*
 * ProtocolCombined.c
 *
 *  Created on: 2016. 5. 16.
 *      Author: leon
 */

//
// *********     Protocol Combined Example      *********
//
//
// Available Dynamixel model on this example : All models using Protocol 1.0 and 2.0
// This example is tested with a Dynamixel MX-28, a Dynamixel PRO 54-200 and an USB2DYNAMIXEL
// Be sure that properties of Dynamixel MX and PRO are already set as %% MX - ID : 1 / Baudnum : 1 (Baudrate : 1000000) , PRO - ID : 1 / Baudnum : 3 (Baudrate : 1000000)
//

// Be aware that:
// This example configures two different control tables (especially, if it uses Dynamixel and Dynamixel PRO). It may modify critical Dynamixel parameter on the control table, if Dynamixels have wrong ID.
//

#ifdef __linux__
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include "DynamixelSDK.h"                                   // Uses Dynamixel SDK library

// Control table address for Dynamixel MX
#define ADDR_MX_TORQUE_ENABLE           24                  // Control table address is different in Dynamixel model
#define ADDR_MX_GOAL_POSITION           30
#define ADDR_MX_PRESENT_POSITION        36

// Control table address for Dynamixel PRO
#define ADDR_PRO_TORQUE_ENABLE          562
#define ADDR_PRO_GOAL_POSITION          596
#define ADDR_PRO_PRESENT_POSITION       611

// Protocol version
#define PROTOCOL_VERSION1               1.0                 // See which protocol version is used in the Dynamixel
#define PROTOCOL_VERSION2               2.0

// Default setting
#define DXL1_ID                         1                   // Dynamixel#1 ID: 1
#define DXL2_ID                         2                   // Dynamixel#2 ID: 2
#define BAUDRATE                        1000000
#define DEVICENAME                      "/dev/ttyUSB0"      // Check which port is being used on your controller
                                                            // ex) Windows: "COM1"   Linux: "/dev/ttyUSB0"

#define TORQUE_ENABLE                   1                   // Value for enabling the torque
#define TORQUE_DISABLE                  0                   // Value for disabling the torque
#define DXL1_MINIMUM_POSITION_VALUE     100                 // Dynamixel will rotate between this value
#define DXL1_MAXIMUM_POSITION_VALUE     4000                // and this value (note that the Dynamixel would not move when the position value is out of movable range. Check e-manual about the range of the Dynamixel you use.)
#define DXL2_MINIMUM_POSITION_VALUE     -150000
#define DXL2_MAXIMUM_POSITION_VALUE     150000
#define DXL1_MOVING_STATUS_THRESHOLD    10                  // Dynamixel MX moving status threshold
#define DXL2_MOVING_STATUS_THRESHOLD    20                  // Dynamixel PRO moving status threshold

#define ESC_ASCII_VALUE                 0x1b

#ifdef __linux__
int _getch()
{
    struct termios oldt, newt;
    int ch;
    tcgetattr( STDIN_FILENO, &oldt );
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr( STDIN_FILENO, TCSANOW, &newt );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
    return ch;
}

int _kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}
#endif

int main()
{
    // Initialize PortHandler Structs
    // Set the port path
    // Get methods and members of PortHandlerLinux or PortHandlerWindows
    int port_num = PortHandler(DEVICENAME);

    // Initialize PacketHandler Structs 
    PacketHandler();

    int index = 0;
    int dxl_comm_result = COMM_TX_FAIL;             // Communication result
    int dxl1_goal_position[2] = {DXL1_MINIMUM_POSITION_VALUE, DXL1_MAXIMUM_POSITION_VALUE};         // Goal position of Dynamixel MX
    int dxl2_goal_position[2] = {DXL2_MINIMUM_POSITION_VALUE, DXL2_MAXIMUM_POSITION_VALUE};         // Goal position of Dynamixel PRO

    UINT8_T dxl_error = 0;                          // Dynamixel error
    UINT16_T dxl1_present_position = 0;             // Present position of Dynamixel MX
    INT32_T dxl2_present_position = 0;              // Present position of Dynamixel PRO

    // Open port
    if (OpenPort(port_num))
    {
        printf("Succeeded to open the port!\n");
    }
    else
    {
        printf("Failed to open the port!\n");
        printf("Press any key to terminate...\n");
        _getch();
        return 0;
    }

    // Set port baudrate
    if (SetBaudRate(port_num, BAUDRATE))
    {
        printf("Succeeded to change the baudrate!\n");
    }
    else
    {
        printf("Failed to change the baudrate!\n");
        printf("Press any key to terminate...\n");
        _getch();
        return 0;
    }

    // Enable Dynamixel#1 torque
    Write1ByteTxRx(port_num, PROTOCOL_VERSION1, DXL1_ID, ADDR_MX_TORQUE_ENABLE, TORQUE_ENABLE);
    if ((dxl_comm_result = GetLastTxRxResult(port_num, PROTOCOL_VERSION1)) != COMM_SUCCESS)
        PrintTxRxResult(PROTOCOL_VERSION1, dxl_comm_result);
    else if ((dxl_error = GetLastRxPacketError(port_num, PROTOCOL_VERSION1)) != 0)
        PrintRxPacketError(PROTOCOL_VERSION1, dxl_error);
    else
        printf("Dynamixel#%d has been successfully connected \n", DXL1_ID);

    // Enable Dynamixel#2 torque
    Write1ByteTxRx(port_num, PROTOCOL_VERSION2, DXL2_ID, ADDR_PRO_TORQUE_ENABLE, TORQUE_ENABLE);
    if ((dxl_comm_result = GetLastTxRxResult(port_num, PROTOCOL_VERSION2)) != COMM_SUCCESS)
        PrintTxRxResult(PROTOCOL_VERSION2, dxl_comm_result);
    else if ((dxl_error = GetLastRxPacketError(port_num, PROTOCOL_VERSION2)) != 0)
        PrintRxPacketError(PROTOCOL_VERSION2, dxl_error);
    else
        printf("Dynamixel#%d has been successfully connected \n", DXL2_ID);

    while(1)
    {
        printf("Press any key to continue! (or press ESC to quit!)\n");
        if(_getch() == ESC_ASCII_VALUE)
            break;

        // Write Dynamixel#1 goal position
        Write2ByteTxRx(port_num, PROTOCOL_VERSION1, DXL1_ID, ADDR_MX_GOAL_POSITION, dxl1_goal_position[index]);
        if ((dxl_comm_result = GetLastTxRxResult(port_num, PROTOCOL_VERSION1)) != COMM_SUCCESS)
            PrintTxRxResult(PROTOCOL_VERSION1, dxl_comm_result);
        else if ((dxl_error = GetLastRxPacketError(port_num, PROTOCOL_VERSION1)) != 0)
            PrintRxPacketError(PROTOCOL_VERSION1, dxl_error);

        // Write Dynamixel#2 goal position
        Write4ByteTxRx(port_num, PROTOCOL_VERSION2, DXL2_ID, ADDR_PRO_GOAL_POSITION, dxl2_goal_position[index]);
        if ((dxl_comm_result = GetLastTxRxResult(port_num, PROTOCOL_VERSION2)) != COMM_SUCCESS)
            PrintTxRxResult(PROTOCOL_VERSION2, dxl_comm_result);
        else if ((dxl_error = GetLastRxPacketError(port_num, PROTOCOL_VERSION2)) != 0)
            PrintRxPacketError(PROTOCOL_VERSION2, dxl_error);

        do
        {
            // Read Dynamixel#1 present position
        	dxl1_present_position = Read2ByteTxRx(port_num, PROTOCOL_VERSION1, DXL1_ID, ADDR_MX_PRESENT_POSITION);
            if ((dxl_comm_result = GetLastTxRxResult(port_num, PROTOCOL_VERSION1)) != COMM_SUCCESS)
                PrintTxRxResult(PROTOCOL_VERSION1, dxl_comm_result);
            else if ((dxl_error = GetLastRxPacketError(port_num, PROTOCOL_VERSION1)) != 0)
                PrintRxPacketError(PROTOCOL_VERSION1, dxl_error);

            // Read Dynamixel#2 present position
            dxl2_present_position = Read4ByteTxRx(port_num, PROTOCOL_VERSION2, DXL2_ID, ADDR_PRO_PRESENT_POSITION);
            if ((dxl_comm_result = GetLastTxRxResult(port_num, PROTOCOL_VERSION2)) != COMM_SUCCESS)
                PrintTxRxResult(PROTOCOL_VERSION2, dxl_comm_result);
            else if ((dxl_error = GetLastRxPacketError(port_num, PROTOCOL_VERSION2)) != 0)
                PrintRxPacketError(PROTOCOL_VERSION2, dxl_error);

            printf("[ID:%03d] GoalPos:%03d  PresPos:%03d [ID:%03d] GoalPos:%03d  PresPos:%03d\n", DXL1_ID, dxl1_goal_position[index], dxl1_present_position, DXL2_ID, dxl2_goal_position[index], dxl2_present_position);

        }while((abs(dxl1_goal_position[index] - dxl1_present_position) > DXL1_MOVING_STATUS_THRESHOLD) || (abs(dxl2_goal_position[index] - dxl2_present_position) > DXL2_MOVING_STATUS_THRESHOLD));

        // Change goal position
        if( index == 0 )
            index = 1;
        else
            index = 0;
    }

    // Disable Dynamixel#1 Torque
    Write1ByteTxRx(port_num, PROTOCOL_VERSION1, DXL1_ID, ADDR_MX_TORQUE_ENABLE, TORQUE_DISABLE);
    if ((dxl_comm_result = GetLastTxRxResult(port_num, PROTOCOL_VERSION1)) != COMM_SUCCESS)
        PrintTxRxResult(PROTOCOL_VERSION1, dxl_comm_result);
    else if ((dxl_error = GetLastRxPacketError(port_num, PROTOCOL_VERSION1)) != 0)
        PrintRxPacketError(PROTOCOL_VERSION1, dxl_error);

    // Disable Dynamixel#2 Torque
    Write1ByteTxRx(port_num, PROTOCOL_VERSION2, DXL2_ID, ADDR_PRO_TORQUE_ENABLE, TORQUE_DISABLE);
    if ((dxl_comm_result = GetLastTxRxResult(port_num, PROTOCOL_VERSION2)) != COMM_SUCCESS)
        PrintTxRxResult(PROTOCOL_VERSION2, dxl_comm_result);
    else if ((dxl_error = GetLastRxPacketError(port_num, PROTOCOL_VERSION2)) != 0)
        PrintRxPacketError(PROTOCOL_VERSION2, dxl_error);

    // Close port
    ClosePort(port_num);

    return 0;
}

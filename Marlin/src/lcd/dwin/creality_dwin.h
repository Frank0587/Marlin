/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

/**
 * DWIN by Creality3D
 * Rewrite and Extui Port by Jacob Myers
 */

#include "dwin.h"
#include "rotary_encoder.h"
#include "../../libs/BL24CXX.h"
#include "../../inc/MarlinConfigPre.h"


enum processID : uint8_t {
  Main, Print, Menu, Value, Option, File, Popup, Confirm, Wait
};

enum PopupID : uint8_t {
  Pause, Stop, Resume, SaveLevel, ETemp, ConfFilChange, PurgeMore,
  Level, Home, MoveWait, Heating,  FilLoad, FilChange, TempWarn, Runout, PIDHotend, PIDBed, Resuming,
  FilInsert, HeaterTime, UserInput, LevelError, InvalidMesh, NocreatePlane, UI, Complete, BadextruderNumber,
  TemptooHigh, PIDTimeout, PIDDone, QMovePosOK, Homingtodo, BaudrateSwitch, MeshSlot 
};

enum menuID : uint8_t {
  MainMenu,
    Prepare,
      Move,
      Quickmove,
      Qmovesettings,
      QmovesettingsA,
      QmovesettingsB,
      QmovesettingsC,
      HomeMenu,
      Sethomeoffsets,
      ManualLevel,
      ZOffset,
      Preheat,
      ChangeFilament,
    Control,
      TempMenu,
        PID,
        HOTENDPID,
        BEDPID,
        Preheat1,
        Preheat2,
        Preheat3,
        Preheat4,
        Preheat5,
      Motion,
        HomeOffsets,
        MaxSpeed,
        MaxAcceleration,
        MaxJerk,
        Steps,
      Visual,
        ColorSettings,
      Advanced,
        ProbeSensor,
        ViewMesh,
      Info,
    Leveling,
      LevelManual,
      LevelView,
      MeshViewer,
      LevelSettings,
      ManualMesh,
      UBLMesh,
    InfoMain,
  Tune,
  PreheatHotend
};


#define Start_Process       0
#define Language_English    1
#define Language_Chinese    2

#define ICON                      0x09
#define ICON_PACK                 0x03
#define ICON_LOGO                  0
#define ICON_Print_0               1
#define ICON_Print_1               2
#define ICON_Prepare_0             3
#define ICON_Prepare_1             4
#define ICON_Control_0             5
#define ICON_Control_1             6
#define ICON_Leveling_0            7
#define ICON_Leveling_1            8
#define ICON_HotendTemp            9
#define ICON_BedTemp              10
#define ICON_Speed                11
#define ICON_Zoffset              12
#define ICON_Back                 13
#define ICON_File                 14
#define ICON_PrintTime            15
#define ICON_RemainTime           16
#define ICON_Setup_0              17
#define ICON_Setup_1              18
#define ICON_Pause_0              19
#define ICON_Pause_1              20
#define ICON_Continue_0           21
#define ICON_Continue_1           22
#define ICON_Stop_0               23
#define ICON_Stop_1               24
#define ICON_Bar                  25
#define ICON_More                 26

#define ICON_Axis                 27
#define ICON_CloseMotor           28
#define ICON_Homing               29
#define ICON_SetHome              30
#define ICON_PLAPreheat           31
#define ICON_ABSPreheat           32
#define ICON_Cool                 33
#define ICON_Language             34

#define ICON_MoveX                35
#define ICON_MoveY                36
#define ICON_MoveZ                37
#define ICON_Extruder             38

#define ICON_Temperature          40
#define ICON_Motion               41
#define ICON_WriteEEPROM          42
#define ICON_ReadEEPROM           43
#define ICON_ResumeEEPROM         44
#define ICON_Info                 45

#define ICON_SetEndTemp           46
#define ICON_SetBedTemp           47
#define ICON_FanSpeed             48
#define ICON_SetPLAPreheat        49
#define ICON_SetABSPreheat        50

#define ICON_MaxSpeed             51
#define ICON_MaxAccelerated       52
#define ICON_MaxJerk              53
#define ICON_Step                 54
#define ICON_PrintSize            55
#define ICON_Version              56
#define ICON_Contact              57
#define ICON_StockConfiguraton    58
#define ICON_MaxSpeedX            59
#define ICON_MaxSpeedY            60
#define ICON_MaxSpeedZ            61
#define ICON_MaxSpeedE            62
#define ICON_MaxAccX              63
#define ICON_MaxAccY              64
#define ICON_MaxAccZ              65
#define ICON_MaxAccE              66
#define ICON_MaxSpeedJerkX        67
#define ICON_MaxSpeedJerkY        68
#define ICON_MaxSpeedJerkZ        69
#define ICON_MaxSpeedJerkE        70
#define ICON_StepX                71
#define ICON_StepY                72
#define ICON_StepZ                73
#define ICON_StepE                74
#define ICON_Setspeed             75
#define ICON_SetZOffset           76
#define ICON_Rectangle            77
#define ICON_BLTouch              78
#define ICON_TempTooLow           79
#define ICON_AutoLeveling         80
#define ICON_TempTooHigh          81
#define ICON_NoTips_C             82
#define ICON_NoTips_E             83
#define ICON_Continue_C           84
#define ICON_Continue_E           85
#define ICON_Cancel_C             86
#define ICON_Cancel_E             87
#define ICON_Confirm_C            88
#define ICON_Confirm_E            89
#define ICON_Info_0               90
#define ICON_Info_1               91

// Custom icons
//#if ENABLED(CREALITY_DWIN_EXTUI_CUSTOM_ICONS)
#if ENABLED(DWIN_CREALITY_LCD_CUSTOM_ICONS)
  // index of every custom icon should be >= CUSTOM_ICON_START
  #define CUSTOM_ICON_START         ICON_Checkbox_F 
  #define ICON_Checkbox_F           200
  #define ICON_Checkbox_T           201
  #define ICON_Fade                 202
  #define ICON_Mesh                 203
  #define ICON_Tilt                 204
  #define ICON_Brightness           205
  #define ICON_Backlight_Off        206
  #define ICON_Probe                207
  #define ICON_Print_A0             210
  #define ICON_Print_A1             211
  #define ICON_Prepare_A0           212
  #define ICON_Prepare_A1           213
  #define ICON_Control_A0           214
  #define ICON_Control_A1           215
  #define ICON_Leveling_A0          216
  #define ICON_Leveling_A1          217
  #define ICON_Setup_A0             218
  #define ICON_Setup_A1             219
  #define ICON_Pause_A0             220
  #define ICON_Pause_A1             221
  #define ICON_Continue_A0          222
  #define ICON_Continue_A1          223
  #define ICON_Stop_A0              224
  #define ICON_Stop_A1              225
  #define ICON_Info_A0              226
  #define ICON_Info_A1              227
  #define ICON_QMove                230
  #define ICON_QMoveTo              231
  #define ICON_SetQMove             232
  #define ICON_QMoveOnlyXY          233
  #define ICON_AxisD                249
  #define ICON_AxisBR               250
  #define ICON_AxisTR               251
  #define ICON_AxisBL               252
  #define ICON_AxisTL               253
  #define ICON_AxisC                254
#else
  #define ICON_Fade                 ICON_Version
  #define ICON_Mesh                 ICON_Version
  #define ICON_Tilt                 ICON_Version
  #define ICON_Brightness           ICON_Version
  #define ICON_Backlight_Off        ICON_Version
  #define ICON_Probe                ICON_StockConfiguraton
  #define ICON_Print_A0             ICON_Print_0
  #define ICON_Print_A1             ICON_Print_1
  #define ICON_Prepare_A0           ICON_Prepare_0
  #define ICON_Prepare_A1           ICON_Prepare_1
  #define ICON_Control_A0           ICON_Control_0
  #define ICON_Control_A1           ICON_Control_1
  #define ICON_Leveling_A0          ICON_Leveling_0
  #define ICON_Leveling_A1          ICON_Leveling_1
  #define ICON_Setup_A0             ICON_Setup_0
  #define ICON_Setup_A1             ICON_Setup_1
  #define ICON_Pause_A0             ICON_Pause_0
  #define ICON_Pause_A1             ICON_Pause_1
  #define ICON_Continue_A0          ICON_Continue_0
  #define ICON_Continue_A1          ICON_Continue_1
  #define ICON_Stop_A0              ICON_Stop_0
  #define ICON_Stop_A1              ICON_Stop_1
  #define ICON_Info_A0              ICON_Info_0
  #define ICON_Info_A1              ICON_Info_1
  #define ICON_QMove                ICON_MaxSpeed
  #define ICON_QMoveTo              ICON_SetHome
  #define ICON_SetQMove             ICON_Motion
  #define ICON_QMoveOnlyXY          ICON_Zoffset
  #define ICON_AxisD                ICON_Axis
  #define ICON_AxisBR               ICON_Axis
  #define ICON_AxisTR               ICON_Axis
  #define ICON_AxisBL               ICON_Axis
  #define ICON_AxisTL               ICON_Axis
  #define ICON_AxisC                ICON_Axis
#endif

// Custom pack icons


#define font6x12  0x00
#define font8x16  0x01
#define font10x20 0x02
#define font12x24 0x03
#define font14x28 0x04
#define font16x32 0x05
#define font20x40 0x06
#define font24x48 0x07
#define font28x56 0x08
#define font32x64 0x09

enum colorID : uint8_t {
  Default, White, Light_White, Blue, Light_Blue, Yellow, Light_Yellow, Orange, Light_Orange, Red, Light_Red, Green, Light_Green, Magenta, Light_Magenta, Cyan, Light_Cyan, Brown, Light_Brown, Black, Grey
};

#define Custom_Colors_no_Black       18
#define Custom_Colors_no_Grey       19
#define Custom_Colors       20
#define Color_White         0xFFFF
#define Color_Light_White   0xBDD7
#define Color_Green         0x07E0
#define Color_Light_Green   0x3460
#define Color_Blue          0x015F
#define Color_Light_Blue    0x3A6A
#define Color_Magenta       0xF81F
#define Color_Light_Magenta 0x9813
#define Color_Red           0xF800
#define Color_Light_Red     0x8800
#define Color_Orange        0xFA20
#define Color_Light_Orange  0xFBC0
#define Color_Yellow        0xFF0F
#define Color_Light_Yellow  0x8BE0
#define Color_Brown         0xCC27
#define Color_Light_Brown   0x6204
#define Color_Black         0x0000
#define Color_Grey          0x18E3
#define Color_Cyan          0x07FF
#define Color_Light_Cyan    0x04F3
#define Color_Bg_Window     0x31E8  // Popup background color
#define Color_Bg_Blue       0x1125  // Dark blue background color
#define Color_Bg_Black      0x0841  // Black background color
#define Color_Bg_Red        0xF00F  // Red background color
#define Popup_Text_Color    0xD6BA  // Popup font background color
#define Line_Color          0x3A6A  // Split line color
#define Rectangle_Color     0xEE2F  // Blue square cursor color
#define Percent_Color       0xFE29  // Percentage color
#define BarFill_Color       0x10E4  // Fill color of progress bar
#define Select_Color        0x33BB  // Selected color
#define Check_Color         0x4E5C  // Check-box check color
#define Confirm_Color   	  0x34B9
#define Cancel_Color        0x3186

class CrealityDWINClass {

public:
  static constexpr size_t eeprom_data_size = 64;
  struct EEPROM_Settings { // use bit fields to save space, max 48 bytes
    bool time_format_textual : 1;
    #if ENABLED(AUTO_BED_LEVELING_UBL)
      uint8_t tilt_grid_size : 3;
    #endif
    uint8_t cursor_color : 5;
    uint8_t menu_split_line : 5;
    uint8_t items_menu_text : 5;
    uint8_t icons_menu_text : 5;
    uint8_t popup_highlight : 5;
    uint8_t popup_text : 5;
    uint8_t popup_bg : 5;
    uint8_t menu_top_bg : 5;
    uint8_t menu_top_txt : 5;
    uint8_t highlight_box : 5;
    uint8_t ico_confirm_txt : 5;
    uint8_t ico_confirm_bg : 5;
    uint8_t ico_cancel_txt : 5;
    uint8_t ico_cancel_bg : 5;
    uint8_t ico_continue_txt : 5;
    uint8_t ico_continue_bg : 5;
    uint8_t print_screen_txt : 5;
    uint8_t print_filename : 5;
    uint8_t progress_bar : 5;
    uint8_t progress_percent : 5;
    uint8_t remain_time : 5;
    uint8_t elapsed_time : 5;
    uint8_t status_bar_text : 5;
    uint8_t status_area_text : 5;
    uint8_t status_area_percent : 5;
    uint8_t coordinates_text : 5;
    uint8_t coordinates_split_line : 5;
    bool beeper_status : 1;
    bool customicons_status : 1;
    bool setoffsets : 1;
    bool LCDFlashed : 1;
    bool only_xy_A : 1;
    bool only_xy_B : 1;
    bool only_xy_C : 1;
    float PositionA_x, PositionA_y, PositionA_z;
    float PositionB_x, PositionB_y, PositionB_z;
    float PositionC_x, PositionC_y, PositionC_z;
    uint8_t baudratemode : 1;
    uint8_t mainiconpack : 1;
  } eeprom_settings;

  const char * const color_names[21] = {"Default","White","L_White","Blue","L_Blue","Yellow","L_Yello","Orange","L_Orang","Red","L_Red","Green","L_Green","Magenta","L_Magen","Cyan","L_Cyan","Brown","L_Brown","Black","Grey"};
  const char * const preheat_modes[3] = {"Both", "Hotend", "Bed"};
  const char * const baudrate_modes[2] = {"250000", "115200"};

  const char * const ico_pack[2] = {"Custom", "Aquila"};

  bool beeperenable = true;
  bool customicons = false;
  uint8_t BAUD_PORT = 0;
  uint8_t brm = 0;

  void Clear_Screen(uint8_t e=3);
  void Draw_Float(float value, uint8_t row, bool selected=false, uint8_t minunit=10);
  void Draw_Option(uint8_t value, const char * const * options, uint8_t row, bool selected=false, bool color=false);
  uint16_t GetColor(uint8_t color, uint16_t original, bool light=false);
  void Draw_Checkbox(uint8_t row, bool value);
  void Draw_Title(const char * title);
  void Draw_Menu_Item(uint8_t row, uint8_t icon=0, bool pack=false, const char * const label1=NULL, const char * const label2=NULL, bool more=false, bool centered=false);
  void Draw_Menu(uint8_t menu, uint8_t select=0, uint8_t scroll=0);
  void Redraw_Menu(bool lastprocess=true, bool lastselection=false, bool lastmenu=false);
  void Redraw_Screen();

  void Main_Menu_Icons();
  void Draw_Main_Menu(uint8_t select=0);
  void Print_Screen_Icons();
  void Draw_Print_Screen();
  void Draw_Print_Filename(bool reset=false);
  void Draw_Print_ProgressBar();
  void Draw_Print_ProgressRemain();
  void Draw_Print_ProgressElapsed();
  void Draw_Print_confirm();
  void Draw_SD_Item(uint8_t item, uint8_t row);
  void Draw_SD_List(bool removed=false);
  void Draw_Status_Area(bool icons=false);
  void Draw_Popup(const char * line1, const char * line2, const char * line3, uint8_t mode, uint8_t icon=0);
  void Popup_Select();
  void Update_Status_Bar(bool refresh=false);

  #if ENABLED(AUTO_BED_LEVELING_UBL)
    void Draw_Bed_Mesh(int16_t selected = -1, uint8_t gridline_width = 1, uint16_t padding_x = 8, uint16_t padding_y_top = 40 + 53 - 7);
    void Set_Mesh_Viewer_Status();
  #endif

  const char * Get_Menu_Title(uint8_t menu);
  uint8_t Get_Menu_Size(uint8_t menu);
  void Menu_Item_Handler(uint8_t menu, uint8_t item, bool draw=true);


  void Popup_Handler(PopupID popupid, bool option = false);
  void Confirm_Handler(PopupID popupid);


  void Main_Menu_Control();
  void Menu_Control();
  void Value_Control();
  void Option_Control();
  void File_Control();
  void Print_Screen_Control();
  void Popup_Control();
  void Confirm_Control();


  void Setup_Value(float value, float min, float max, float unit, uint8_t type);
  void Modify_Value(float &value, float min, float max, float unit, void (*f)()=NULL);
  void Modify_Value(uint8_t &value, float min, float max, float unit, void (*f)()=NULL);
  void Modify_Value(uint16_t &value, float min, float max, float unit, void (*f)()=NULL);
  void Modify_Value(int16_t &value, float min, float max, float unit, void (*f)()=NULL);
  void Modify_Value(uint32_t &value, float min, float max, float unit, void (*f)()=NULL);
  void Modify_Value(int8_t &value, float min, float max, float unit, void (*f)()=NULL);
  void Modify_Option(uint8_t value, const char * const * options, uint8_t max);


  void Update_Status(const char * const text);
  void Start_Print(bool sd);
  void Stop_Print();
  void Update();
  void State_Update();
  void Screen_Update();
  void AudioFeedback(const bool success=true);
  void Save_Settings(char *buff);
  void Load_Settings(const char *buff);

  enum result_t   : uint8_t { PID_STARTED, PID_BAD_EXTRUDER_NUM, PID_TEMP_TOO_HIGH, PID_TUNING_TIMEOUT, PID_DONE };
  void onPidTuning(const result_t rst);

  void Quick_Move_Item_Menu(uint8_t row, float pos_x, float pos_y, float pos_z, bool only_xy);
  void Quick_Move(float pos_x, float pos_y, float pos_z, bool only_xy);
  float pos_xx, pos_yy, pos_zz;
  bool only_xxyy;
};

extern CrealityDWINClass CrealityDWIN;
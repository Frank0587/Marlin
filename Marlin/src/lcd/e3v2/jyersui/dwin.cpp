/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2021 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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

/**
 * lcd/e3v2/jyersui/dwin.cpp
 */

#include "../../../inc/MarlinConfigPre.h"

#if ENABLED(DWIN_CREALITY_LCD_JYERSUI)

#include "dwin.h"

#include "../../marlinui.h"
#include "../../../MarlinCore.h"

#include "../../../gcode/gcode.h"
#include "../../../module/temperature.h"
#include "../../../module/planner.h"
#include "../../../module/settings.h"
#include "../../../libs/buzzer.h"
#include "../../../inc/Conditionals_post.h"

#define DEBUG_OUT ENABLED(DEBUG_LCD_UI)
#include "../../../core/debug_out.h"

#if ENABLED(ADVANCED_PAUSE_FEATURE)
  #include "../../../feature/pause.h"
#endif

#if ENABLED(FILAMENT_RUNOUT_SENSOR)
  #include "../../../feature/runout.h"
#endif

#if ENABLED(HOST_ACTION_COMMANDS)
  #include "../../../feature/host_actions.h"
#endif

#if ANY(BABYSTEPPING, HAS_BED_PROBE, HAS_WORKSPACE_OFFSET)
  #define HAS_ZOFFSET_ITEM 1
#endif

#ifndef strcasecmp_P
  #define strcasecmp_P(a, b) strcasecmp((a), (b))
#endif

#ifdef BLTOUCH_HS_MODE
  #include "../../../feature/bltouch.h"
#endif

#if HAS_LEVELING
  #include "../../../feature/bedlevel/bedlevel.h"
#endif

#if ENABLED(AUTO_BED_LEVELING_UBL)
  #include "../../../libs/least_squares_fit.h"
  #include "../../../libs/vector_3.h"
#endif

#if HAS_BED_PROBE
  #include "../../../module/probe.h"
#endif

#if ENABLED(POWER_LOSS_RECOVERY)
  #include "../../../feature/powerloss.h"
#endif

#if ENABLED(DWIN_CREALITY_LCD_JYERSUI_GCODE_PREVIEW)
  #include "../../../libs/base64.hpp"
  #include <map>
  #include <string>
  using namespace std;
#endif

#define MACHINE_SIZE STRINGIFY(X_BED_SIZE) "x" STRINGIFY(Y_BED_SIZE) "x" STRINGIFY(Z_MAX_POS)

#define DWIN_FONT_MENU font8x16
#define DWIN_FONT_STAT font10x20
#define DWIN_FONT_HEAD font10x20

#define MENU_CHAR_LIMIT  24
#define STATUS_CHAR_LIMIT  30
#define STATUS_Y 352

#define MAX_PRINT_SPEED   500
#define MIN_PRINT_SPEED   10

#if HAS_FAN
  #define MAX_FAN_SPEED     255
  #define MIN_FAN_SPEED     0
#endif

#define MAX_XY_OFFSET 100

#if HAS_ZOFFSET_ITEM
  #define MAX_Z_OFFSET 9.99
  #if HAS_BED_PROBE
    #define MIN_Z_OFFSET -9.99
  #else
    #define MIN_Z_OFFSET -1
  #endif
#endif

#if HAS_HOTEND
  #define MAX_FLOW_RATE   200
  #define MIN_FLOW_RATE   10

  #define MAX_E_TEMP    (HEATER_0_MAXTEMP - HOTEND_OVERSHOOT)
  #define MIN_E_TEMP    0
#endif

#if HAS_HEATED_BED
  #define MAX_BED_TEMP  BED_MAXTEMP
  #define MIN_BED_TEMP  0
#endif

constexpr uint16_t TROWS = 6, MROWS = TROWS - 1,
                   TITLE_HEIGHT = 30,
                   MLINE = 53,
                   LBLX = 60,
                   MENU_CHR_W = 8, MENU_CHR_H = 16, STAT_CHR_W = 10;

#define KEY_WIDTH 26
#define KEY_HEIGHT 30
#define KEY_INSET 5
#define KEY_PADDING 3
#define KEY_Y_START DWIN_HEIGHT-(KEY_HEIGHT*4+2*(KEY_INSET+1))

#define MBASE(L) (49 + MLINE * (L))

constexpr float default_max_feedrate[]        = DEFAULT_MAX_FEEDRATE;
constexpr float default_max_acceleration[]    = DEFAULT_MAX_ACCELERATION;
constexpr float default_steps[]               = DEFAULT_AXIS_STEPS_PER_UNIT;
#if HAS_CLASSIC_JERK
  constexpr float default_max_jerk[]            = { DEFAULT_XJERK, DEFAULT_YJERK, DEFAULT_ZJERK, DEFAULT_EJERK };
#endif

enum SelectItem : uint8_t {
  PAGE_PRINT = 0,
  PAGE_PREPARE,
  PAGE_CONTROL,
  PAGE_INFO_LEVELING,
  PAGE_COUNT,

  PRINT_SETUP = 0,
  PRINT_PAUSE_RESUME,
  PRINT_STOP,
  PRINT_COUNT
};

uint8_t active_menu = MainMenu, last_menu = MainMenu;
uint8_t selection = 0, last_selection = 0;
uint8_t scrollpos = 0;
uint8_t process = Main, last_process = Main;
PopupID popup, last_popup;
bool keyboard_restrict, reset_keyboard, numeric_keyboard = false;
uint8_t maxstringlen;
char *stringpointer = nullptr;

void (*funcpointer)() = nullptr;
void *valuepointer = nullptr;
float tempvalue;
float valuemin;
float valuemax;
uint8_t valueunit;
uint8_t valuetype;

char cmd[MAX_CMD_SIZE+16], str_1[16], str_2[16], str_3[16];
char statusmsg[64];
char filename[LONG_FILENAME_LENGTH];
bool printing = false;
bool paused = false;
bool sdprint = false;

int16_t pausetemp, pausebed, pausefan;

bool livemove = false;
bool liveadjust = false;
uint8_t preheatmode = 0;
float zoffsetvalue = 0;
uint8_t gridpoint;
float corner_avg;
float corner_pos;

#if ENABLED(HOST_ACTION_COMMANDS)
  char action1[9];
  char action2[9];
  char action3[9];
#endif

bool probe_deployed = false;

#if ENABLED(DWIN_CREALITY_LCD_JYERSUI_GCODE_PREVIEW)
  std::map<string, int> image_cache;
  uint16_t next_available_address = 1;
  static millis_t thumbtime = 0;
  static millis_t name_scroll_time = 0;
  #define SCROLL_WAIT 1000
#endif


#ifdef DEBUG_LCD_UI
  #define DEBUG_INFOLINE(a)  dbg_UpdateInfoLine(a)

  char dbg_InfoLine1[64], dbg_InfoLine2[64];

  bool dbg_UpdateInfoLine (uint8_t idx) {
    bool rtn = false;
    static uint8_t fresh = 0;
    static uint64_t lastHash = 0;
    uint8_t timer = print_job_timer.isPaused() + 2* print_job_timer.isRunning();
    uint64_t hash =
            (printing                    ? 1 : 0)
          + (paused                      ? 2 : 0)
          + (wait_for_user               ? 4 : 0)
          + 0x000000000010 * timer
          + 0x000000000100 * process
          + 0x000000010000 * last_process
          + 0x000001000000 * selection
          + 0x000100000000 * last_selection
          + 0x010000000000 * pause_menu_response;

    if (lastHash != hash) {
      lastHash = hash;
      fresh = 0xFF;
      rtn = true;

      sprintf_P(dbg_InfoLine1, PSTR("prc:%i/%i|prt:%i|pau=%i|tim:%i|wfu:%i|sel:%i/%i"),
              process, last_process, printing, paused, timer, wait_for_user, selection, last_selection );
      sprintf_P(dbg_InfoLine2, PSTR(" - was geht up (line 2) - "));
    }
    if (idx > 7) idx = 0;
    if (fresh & (1<<idx)) {
      fresh &= ~(1<<idx);
      rtn = true;
    }
    return rtn;
  }
#else
  #define DEBUG_INFOLINE(a) false
#endif


CrealityDWINClass CrealityDWIN;

#if HAS_MESH

  struct Mesh_Settings {
    bool viewer_asymmetric_range = false;
    bool viewer_print_value = false;
    bool goto_mesh_value = false;
    bool drawing_mesh = false;
    uint8_t mesh_x = 0;
    uint8_t mesh_y = 0;

    #if ENABLED(AUTO_BED_LEVELING_UBL)
      uint8_t tilt_grid = 1;

      void manual_value_update(bool undefined=false) {
        sprintf_P(cmd, PSTR("M421 I%i J%i Z%s %s"), mesh_x, mesh_y, dtostrf(current_position.z, 1, 3, str_1), undefined ? "N" : "");
        gcode.process_subcommands_now(cmd);
        planner.synchronize();
      }

      bool create_plane_from_mesh() {
        struct linear_fit_data lsf_results;
        incremental_LSF_reset(&lsf_results);
        GRID_LOOP(x, y) {
          if (!isnan(Z_VALUES_ARR[x][y])) {
            xy_pos_t rpos;
            rpos.x = ubl.mesh_index_to_xpos(x);
            rpos.y = ubl.mesh_index_to_ypos(y);
            incremental_LSF(&lsf_results, rpos, Z_VALUES_ARR[x][y]);
          }
        }

        if (finish_incremental_LSF(&lsf_results)) {
          SERIAL_ECHOPGM("Could not complete LSF!");
          return true;
        }

        ubl.set_all_mesh_points_to_value(0);

        matrix_3x3 rotation = matrix_3x3::create_look_at(vector_3(lsf_results.A, lsf_results.B, 1));
        GRID_LOOP(i, j) {
          float mx = ubl.mesh_index_to_xpos(i),
                my = ubl.mesh_index_to_ypos(j),
                mz = Z_VALUES_ARR[i][j];

          if (DEBUGGING(LEVELING)) {
            DEBUG_ECHOPAIR_F("before rotation = [", mx, 7);
            DEBUG_CHAR(',');
            DEBUG_ECHO_F(my, 7);
            DEBUG_CHAR(',');
            DEBUG_ECHO_F(mz, 7);
            DEBUG_ECHOPGM("]   ---> ");
            DEBUG_DELAY(20);
          }

          rotation.apply_rotation_xyz(mx, my, mz);

          if (DEBUGGING(LEVELING)) {
            DEBUG_ECHOPAIR_F("after rotation = [", mx, 7);
            DEBUG_CHAR(',');
            DEBUG_ECHO_F(my, 7);
            DEBUG_CHAR(',');
            DEBUG_ECHO_F(mz, 7);
            DEBUG_ECHOLNPGM("]");
            DEBUG_DELAY(20);
          }

          Z_VALUES_ARR[i][j] = mz - lsf_results.D;
        }
        return false;
      }

    #else

      void manual_value_update() {
        sprintf_P(cmd, PSTR("G29 I%i J%i Z%s"), mesh_x, mesh_y, dtostrf(current_position.z, 1, 3, str_1));
        gcode.process_subcommands_now(cmd);
        planner.synchronize();
      }

    #endif

    void manual_move(bool zmove=false) {
      if (zmove) {
        planner.synchronize();
        current_position.z = goto_mesh_value ? Z_VALUES_ARR[mesh_x][mesh_y] : Z_CLEARANCE_BETWEEN_PROBES;
        planner.buffer_line(current_position, homing_feedrate(Z_AXIS), active_extruder);
        planner.synchronize();
      }
      else {
        CrealityDWIN.Popup_Handler(MoveWait);
        sprintf_P(cmd, PSTR("G0 F300 Z%s"), dtostrf(Z_CLEARANCE_BETWEEN_PROBES, 1, 3, str_1));
        gcode.process_subcommands_now(cmd);
        sprintf_P(cmd, PSTR("G42 F4000 I%i J%i"), mesh_x, mesh_y);
        gcode.process_subcommands_now(cmd);
        planner.synchronize();
        current_position.z = goto_mesh_value ? Z_VALUES_ARR[mesh_x][mesh_y] : Z_CLEARANCE_BETWEEN_PROBES;
        planner.buffer_line(current_position, homing_feedrate(Z_AXIS), active_extruder);
        planner.synchronize();
        CrealityDWIN.Redraw_Menu();
      }
    }

    float get_max_value() {
      float max = __FLT_MIN__;
      GRID_LOOP(x, y) {
        if (!isnan(Z_VALUES_ARR[x][y]) && Z_VALUES_ARR[x][y] > max)
          max = Z_VALUES_ARR[x][y];
      }
      return max;
    }

    float get_min_value() {
      float min = __FLT_MAX__;
      GRID_LOOP(x, y) {
        if (!isnan(Z_VALUES_ARR[x][y]) && Z_VALUES_ARR[x][y] < min)
          min = Z_VALUES_ARR[x][y];
      }
      return min;
    }

    void Draw_Bed_Mesh(int16_t selected = -1, uint8_t gridline_width = 1, uint16_t padding_x = 8, uint16_t padding_y_top = 40 + 53 - 7) {
      drawing_mesh = true;
      const uint16_t total_width_px = DWIN_WIDTH - padding_x - padding_x;
      const uint16_t cell_width_px  = total_width_px / GRID_MAX_POINTS_X;
      const uint16_t cell_height_px = total_width_px / GRID_MAX_POINTS_Y;
      const float v_max = abs(get_max_value()), v_min = abs(get_min_value()), range = _MAX(v_min, v_max);

      // Clear background from previous selection and select new square
      DWIN_Draw_Rectangle(1, Color_Bg_Black, _MAX(0, padding_x - gridline_width), _MAX(0, padding_y_top - gridline_width), padding_x + total_width_px, padding_y_top + total_width_px);
      if (selected >= 0) {
        const auto selected_y = selected / GRID_MAX_POINTS_X;
        const auto selected_x = selected - (GRID_MAX_POINTS_X * selected_y);
        const auto start_y_px = padding_y_top + selected_y * cell_height_px;
        const auto start_x_px = padding_x + selected_x * cell_width_px;
        DWIN_Draw_Rectangle(1, Color_White, _MAX(0, start_x_px - gridline_width), _MAX(0, start_y_px - gridline_width), start_x_px + cell_width_px, start_y_px + cell_height_px);
      }

      // Draw value square grid
      char buf[8];
      GRID_LOOP(x, y) {
        const auto start_x_px = padding_x + x * cell_width_px;
        const auto end_x_px   = start_x_px + cell_width_px - 1 - gridline_width;
        const auto start_y_px = padding_y_top + (GRID_MAX_POINTS_Y - y - 1) * cell_height_px;
        const auto end_y_px   = start_y_px + cell_height_px - 1 - gridline_width;
        DWIN_Draw_Rectangle(1,                                                                                 // RGB565 colors: http://www.barth-dev.de/online/rgb565-color-picker/
          isnan(Z_VALUES_ARR[x][y]) ? Color_Grey : (                                                           // gray if undefined
            (Z_VALUES_ARR[x][y] < 0 ?
              (uint16_t)round(0x1F * -Z_VALUES_ARR[x][y] / (!viewer_asymmetric_range ? range : v_min)) << 11 : // red if mesh point value is negative
              (uint16_t)round(0x3F *  Z_VALUES_ARR[x][y] / (!viewer_asymmetric_range ? range : v_max)) << 5) | // green if mesh point value is positive
                _MIN(0x1F, (((uint8_t)abs(Z_VALUES_ARR[x][y]) / 10) * 4))),                                    // + blue stepping for every mm
          start_x_px, start_y_px, end_x_px, end_y_px
        );

        safe_delay(10);
        LCD_SERIAL.flushTX();

        // Draw value text on
        if (viewer_print_value) {
          int8_t offset_x, offset_y = cell_height_px / 2 - 6;
          if (isnan(Z_VALUES_ARR[x][y])) {  // undefined
            DWIN_Draw_String(false, font6x12, Color_White, Color_Bg_Blue, start_x_px + cell_width_px / 2 - 5, start_y_px + offset_y, F("X"));
          }
          else {                          // has value
            if (GRID_MAX_POINTS_X < 10)
              sprintf_P(buf, PSTR("%s"), dtostrf(abs(Z_VALUES_ARR[x][y]), 1, 2, str_1));
            else
              sprintf_P(buf, PSTR("%02i"), (uint16_t)(abs(Z_VALUES_ARR[x][y] - (int16_t)Z_VALUES_ARR[x][y]) * 100));
            offset_x = cell_width_px / 2 - 3 * (strlen(buf)) - 2;
            if (!(GRID_MAX_POINTS_X < 10))
              DWIN_Draw_String(false, font6x12, Color_White, Color_Bg_Blue, start_x_px - 2 + offset_x, start_y_px + offset_y /*+ square / 2 - 6*/, F("."));
            DWIN_Draw_String(false, font6x12, Color_White, Color_Bg_Blue, start_x_px + 1 + offset_x, start_y_px + offset_y /*+ square / 2 - 6*/, buf);
          }
          safe_delay(10);
          LCD_SERIAL.flushTX();
        }
      }
    }

    void Set_Mesh_Viewer_Status() { // TODO: draw gradient with values as a legend instead
      float v_max = abs(get_max_value()), v_min = abs(get_min_value()), range = _MAX(v_min, v_max);
      if (v_min > 3e+10F) v_min = 0.0000001;
      if (v_max > 3e+10F) v_max = 0.0000001;
      if (range > 3e+10F) range = 0.0000001;
      char msg[46];
      if (viewer_asymmetric_range) {
        dtostrf(-v_min, 1, 3, str_1);
        dtostrf( v_max, 1, 3, str_2);
      }
      else {
        dtostrf(-range, 1, 3, str_1);
        dtostrf( range, 1, 3, str_2);
      }
      sprintf_P(msg, PSTR("Red %s..0..%s Green"), str_1, str_2);
      CrealityDWIN.Update_Status(msg);
      drawing_mesh = false;
    }

  };
  Mesh_Settings mesh_conf;

#endif // HAS_MESH

/* General Display Functions */

struct CrealityDWINClass::EEPROM_Settings CrealityDWINClass::eeprom_settings{0};
constexpr const char * const CrealityDWINClass::color_names[11];
constexpr const char * const CrealityDWINClass::preheat_modes[3];

// Clear a part of the screen
//  4=Entire screen
//  3=Title bar and Menu area (default)
//  2=Menu area
//  1=Title bar
void CrealityDWINClass::Clear_Screen(uint8_t e/*=3*/) {
  if (e == 1 || e == 3 || e == 4) DWIN_Draw_Rectangle(1, GetColor(eeprom_settings.menu_top_bg, Color_Bg_Blue, false), 0, 0, DWIN_WIDTH, TITLE_HEIGHT); // Clear Title Bar
  if (e == 2 || e == 3) DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, 31, DWIN_WIDTH, STATUS_Y); // Clear Menu Area
  if (e == 4) DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, 31, DWIN_WIDTH, DWIN_HEIGHT); // Clear Popup Area
}

void CrealityDWINClass::Draw_Float(float value, uint8_t row, bool selected/*=false*/, uint8_t minunit/*=10*/) {
  const uint8_t digits = (uint8_t)floor(log10(abs(value))) + log10(minunit) + (minunit > 1);
  const uint16_t bColor = (selected) ? Select_Color : Color_Bg_Black;
  const uint16_t xpos = 240 - (digits * 8);
  DWIN_Draw_Rectangle(1, Color_Bg_Black, 194, MBASE(row), 234 - (digits * 8), MBASE(row) + 16);
  if (isnan(value))
    DWIN_Draw_String(true, DWIN_FONT_MENU, Color_White, bColor, xpos - 8, MBASE(row), F(" NaN"));
  else {
    DWIN_Draw_FloatValue(true, true, 0, DWIN_FONT_MENU, Color_White, bColor, digits - log10(minunit) + 1, log10(minunit), xpos, MBASE(row), (value < 0 ? -value : value));
    DWIN_Draw_String(true, DWIN_FONT_MENU, Color_White, bColor, xpos - 8, MBASE(row), value < 0 ? F("-") : F(" "));
  }
}

void CrealityDWINClass::Draw_Option(uint8_t value, const char * const * options, uint8_t row, bool selected/*=false*/, bool color/*=false*/) {
  uint16_t bColor = (selected) ? Select_Color : Color_Bg_Black;
  uint16_t tColor = (color) ? GetColor(value, Color_White, false) : Color_White;
  DWIN_Draw_Rectangle(1, bColor, 202, MBASE(row) + 14, 258, MBASE(row) - 2);
  DWIN_Draw_String(false, DWIN_FONT_MENU, tColor, bColor, 202, MBASE(row) - 1, options[value]);
}

void CrealityDWINClass::Draw_String(char * string, uint8_t row, bool selected/*=false*/, bool below/*=false*/) {
  if (!string) string[0] = '\0';
  const uint8_t offset_x = DWIN_WIDTH-strlen(string)*8 - 20;
  const uint8_t offset_y = (below) ? MENU_CHR_H * 3 / 5 : 0;
  DWIN_Draw_Rectangle(1, Color_Bg_Black, offset_x - 10, MBASE(row)+offset_y-1, offset_x, MBASE(row)+16+offset_y);
  DWIN_Draw_String(true, DWIN_FONT_MENU, Color_White, (selected) ? Select_Color : Color_Bg_Black, offset_x, MBASE(row)-1+offset_y, string);
}

const uint64_t CrealityDWINClass::Encode_String(const char * string) {
  const char table[65] = "-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
  uint64_t output = 0;
  LOOP_L_N(i, strlen(string)) {
    uint8_t upper_bound = 63, lower_bound = 0;
    uint8_t midpoint;
    LOOP_L_N(x, 6) {
      midpoint = (uint8_t)(0.5*(upper_bound+lower_bound));
      if (string[i] > table[midpoint]) lower_bound = midpoint;
      else if (string[i] < table[midpoint]) upper_bound = midpoint;
      else break;
    }
    output += midpoint*pow(64,i);
  }
  return output;
}

void CrealityDWINClass::Decode_String(uint64_t num, char * string) {
  const char table[65] = "-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
  LOOP_L_N(i, 30) {
    string[i] = table[num%64];
    num /= 64;
    if (num==0) {
      string[i+1] = '\0';
      break;
    }
  }
}


uint16_t CrealityDWINClass::GetColor(uint8_t color, uint16_t original, bool light/*=false*/) {
  switch (color){
    case Default:
      return original;
      break;
    case White:
      return (light) ? Color_Light_White : Color_White;
      break;
    case Green:
      return (light) ? Color_Light_Green : Color_Green;
      break;
    case Cyan:
      return (light) ? Color_Light_Cyan : Color_Cyan;
      break;
    case Blue:
      return (light) ? Color_Light_Blue : Color_Blue;
      break;
    case Magenta:
      return (light) ? Color_Light_Magenta : Color_Magenta;
      break;
    case Red:
      return (light) ? Color_Light_Red : Color_Red;
      break;
    case Orange:
      return (light) ? Color_Light_Orange : Color_Orange;
      break;
    case Yellow:
      return (light) ? Color_Light_Yellow : Color_Yellow;
      break;
    case Brown:
      return (light) ? Color_Light_Brown : Color_Brown;
      break;
    case Black:
      return Color_Black;
      break;
  }
  return Color_White;
}

void CrealityDWINClass::Draw_Title(const char * ctitle) {
  DWIN_Draw_String(false, DWIN_FONT_HEAD, GetColor(eeprom_settings.menu_top_txt, Color_White, false), Color_Bg_Blue, (DWIN_WIDTH - strlen(ctitle) * STAT_CHR_W) / 2, 5, ctitle);
}
void CrealityDWINClass::Draw_Title(FSTR_P const ftitle) {
  DWIN_Draw_String(false, DWIN_FONT_HEAD, GetColor(eeprom_settings.menu_top_txt, Color_White, false), Color_Bg_Blue, (DWIN_WIDTH - strlen_P(FTOP(ftitle)) * STAT_CHR_W) / 2, 5, ftitle);
}

void _Decorate_Menu_Item(uint8_t row, uint8_t icon, bool more) {
  if (icon) DWIN_ICON_Show(ICON, icon, 26, MBASE(row) - 3);   //Draw Menu Icon
  if (more) DWIN_ICON_Show(ICON, ICON_More, 226, MBASE(row) - 3); // Draw More Arrow
  DWIN_Draw_Line(CrealityDWIN.GetColor(CrealityDWIN.eeprom_settings.menu_split_line, Line_Color, true), 16, MBASE(row) + 33, 256, MBASE(row) + 33); // Draw Menu Line
}

uint16_t image_address;
void CrealityDWINClass::Draw_Menu_Item(uint16_t row, uint8_t icon/*=0*/, const char * label1, const char * label2, bool more/*=false*/, bool centered/*=false*/, bool onlyCachedFileIcon/*=false*/) {
  const uint8_t label_offset_y = (label1 && label2) ? MENU_CHR_H * 3 / 5 : 0,
                label1_offset_x = !centered ? LBLX : LBLX * 4/5 + _MAX(LBLX * 1U/5, (DWIN_WIDTH - LBLX - (label1 ? strlen(label1) : 0) * MENU_CHR_W) / 2),
                label2_offset_x = !centered ? LBLX : LBLX * 4/5 + _MAX(LBLX * 1U/5, (DWIN_WIDTH - LBLX - (label2 ? strlen(label2) : 0) * MENU_CHR_W) / 2);
  if (label1) DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, label1_offset_x, MBASE(row) - 1 - label_offset_y, label1); // Draw Label
  if (label2) DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, label2_offset_x, MBASE(row) - 1 + label_offset_y, label2); // Draw Label
  //_Decorate_Menu_Item(row, icon, more);
  #ifdef DWIN_CREALITY_LCD_JYERSUI_GCODE_PREVIEW
   if (eeprom_settings.show_gcode_thumbnails && icon == ICON_File && find_and_decode_gcode_preview(card.filename, Thumnail_Icon, &image_address, onlyCachedFileIcon))
    DWIN_SRAM_Memory_Icon_Display(9, MBASE(row) - 18, image_address);
   else if (icon) DWIN_ICON_Show(ICON, icon, 26, MBASE(row) - 3);   //Draw Menu Icon
  #else
   if (icon) DWIN_ICON_Show(ICON, icon, 26, MBASE(row) - 3);   //Draw Menu Icon
  #endif
   if (more) DWIN_ICON_Show(ICON, ICON_More, 226, MBASE(row) - 3); // Draw More Arrow
  DWIN_Draw_Line(GetColor(eeprom_settings.menu_split_line, Line_Color, true), 16, MBASE(row) + 33, 256, MBASE(row) + 33); // Draw Menu Line
  }

void CrealityDWINClass::Draw_Menu_Item(uint8_t row, uint8_t icon/*=0*/, FSTR_P const flabel1, FSTR_P const flabel2, bool more/*=false*/, bool centered/*=false*/, bool onlyCachedFileIcon/*=false*/) {
  const uint8_t label_offset_y = (flabel1 && flabel2) ? MENU_CHR_H * 3 / 5 : 0,
                label1_offset_x = !centered ? LBLX : LBLX * 4/5 + _MAX(LBLX * 1U/5, (DWIN_WIDTH - LBLX - (flabel1 ? strlen_P(FTOP(flabel1)) : 0) * MENU_CHR_W) / 2),
                label2_offset_x = !centered ? LBLX : LBLX * 4/5 + _MAX(LBLX * 1U/5, (DWIN_WIDTH - LBLX - (flabel2 ? strlen_P(FTOP(flabel2)) : 0) * MENU_CHR_W) / 2);
  if (flabel1) DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, label1_offset_x, MBASE(row) - 1 - label_offset_y, flabel1); // Draw Label
  if (flabel2) DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, label2_offset_x, MBASE(row) - 1 + label_offset_y, flabel2); // Draw Label
  //_Decorate_Menu_Item(row, icon, more);
  #ifdef DWIN_CREALITY_LCD_JYERSUI_GCODE_PREVIEW
   if (eeprom_settings.show_gcode_thumbnails && icon == ICON_File && find_and_decode_gcode_preview(card.filename, Thumnail_Icon, &image_address, onlyCachedFileIcon))
    DWIN_SRAM_Memory_Icon_Display(9, MBASE(row) - 18, image_address);
   else if (icon) DWIN_ICON_Show(ICON, icon, 26, MBASE(row) - 3);   //Draw Menu Icon
  #else
   if (icon) DWIN_ICON_Show(ICON, icon, 26, MBASE(row) - 3);   //Draw Menu Icon
  #endif
   if (more) DWIN_ICON_Show(ICON, ICON_More, 226, MBASE(row) - 3); // Draw More Arrow
  DWIN_Draw_Line(GetColor(eeprom_settings.menu_split_line, Line_Color, true), 16, MBASE(row) + 33, 256, MBASE(row) + 33); // Draw Menu Line
}

void CrealityDWINClass::Draw_Checkbox(uint8_t row, bool value) {
  #if ENABLED(DWIN_CREALITY_LCD_CUSTOM_ICONS) // Draw appropriate checkbox icon
    DWIN_ICON_Show(ICON, (value ? ICON_Checkbox_T : ICON_Checkbox_F), 226, MBASE(row) - 3);
  #else                                         // Draw a basic checkbox using rectangles and lines
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 226, MBASE(row) - 3, 226 + 20, MBASE(row) - 3 + 20);
    DWIN_Draw_Rectangle(0, Color_White, 226, MBASE(row) - 3, 226 + 20, MBASE(row) - 3 + 20);
    if (value) {
      DWIN_Draw_Line(Check_Color, 227, MBASE(row) - 3 + 11, 226 + 8, MBASE(row) - 3 + 17);
      DWIN_Draw_Line(Check_Color, 227 + 8, MBASE(row) - 3 + 17, 226 + 19, MBASE(row) - 3 + 1);
      DWIN_Draw_Line(Check_Color, 227, MBASE(row) - 3 + 12, 226 + 8, MBASE(row) - 3 + 18);
      DWIN_Draw_Line(Check_Color, 227 + 8, MBASE(row) - 3 + 18, 226 + 19, MBASE(row) - 3 + 2);
      DWIN_Draw_Line(Check_Color, 227, MBASE(row) - 3 + 13, 226 + 8, MBASE(row) - 3 + 19);
      DWIN_Draw_Line(Check_Color, 227 + 8, MBASE(row) - 3 + 19, 226 + 19, MBASE(row) - 3 + 3);
    }
  #endif
}

void CrealityDWINClass::Draw_Menu(uint8_t menu, uint8_t select/*=0*/, uint8_t scroll/*=0*/) {
  if (active_menu != menu) {
    last_menu = active_menu;
    if (process == Menu) last_selection = selection;
  }
  selection = _MIN(select, Get_Menu_Size(menu));
  scrollpos = scroll;
  if (selection - scrollpos > MROWS)
    scrollpos = selection - MROWS;
  process = Menu;
  active_menu = menu;
  Clear_Screen();
  Draw_Title(Get_Menu_Title(menu));
  LOOP_L_N(i, TROWS) Menu_Item_Handler(menu, i + scrollpos);
  DWIN_Draw_Rectangle(1, GetColor(eeprom_settings.cursor_color, Rectangle_Color), 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
}

void CrealityDWINClass::Redraw_Menu(bool lastprocess/*=true*/, bool lastselection/*=false*/, bool lastmenu/*=false*/) {
  switch ((lastprocess) ? last_process : process) {
    case Menu:
      Draw_Menu((lastmenu) ? last_menu : active_menu, (lastselection) ? last_selection : selection, (lastmenu) ? 0 : scrollpos);
      break;
    case Main:  Draw_Main_Menu((lastselection) ? last_selection : selection); break;
    case Print: Draw_Print_Screen(); break;
    case File:  Draw_SD_List(); break;
    default: break;
  }
}

void CrealityDWINClass::Redraw_Screen() {
  Redraw_Menu(false);
  Draw_Status_Area(true);
  Update_Status_Bar(true);
}

/* Primary Menus and Screen Elements */

void CrealityDWINClass::Main_Menu_Icons() {
  if (selection == 0) {
    DWIN_ICON_Show(ICON, ICON_Print_1, 17, 130);
    DWIN_Draw_Rectangle(0, GetColor(eeprom_settings.highlight_box, Color_White), 17, 130, 126, 229);
    DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 52, 200, F("Print"));
  }
  else {
    DWIN_ICON_Show(ICON, ICON_Print_0, 17, 130);
    DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 52, 200, F("Print"));
  }
  if (selection == 1) {
    DWIN_ICON_Show(ICON, ICON_Prepare_1, 145, 130);
    DWIN_Draw_Rectangle(0, GetColor(eeprom_settings.highlight_box, Color_White), 145, 130, 254, 229);
    DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 170, 200, F("Prepare"));
  }
  else {
    DWIN_ICON_Show(ICON, ICON_Prepare_0, 145, 130);
    DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 170, 200, F("Prepare"));
  }
  if (selection == 2) {
    DWIN_ICON_Show(ICON, ICON_Control_1, 17, 246);
    DWIN_Draw_Rectangle(0, GetColor(eeprom_settings.highlight_box, Color_White), 17, 246, 126, 345);
    DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 43, 317, F("Control"));
  }
  else {
    DWIN_ICON_Show(ICON, ICON_Control_0, 17, 246);
    DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 43, 317, F("Control"));
  }
  #if HAS_ABL_OR_UBL
    if (selection == 3) {
      DWIN_ICON_Show(ICON, ICON_Leveling_1, 145, 246);
      DWIN_Draw_Rectangle(0, GetColor(eeprom_settings.highlight_box, Color_White), 145, 246, 254, 345);
      DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 179, 317, F("Level"));
    }
    else {
      DWIN_ICON_Show(ICON, ICON_Leveling_0, 145, 246);
      DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 179, 317, F("Level"));
    }
  #else
    if (selection == 3) {
      DWIN_ICON_Show(ICON, ICON_Info_1, 145, 246);
      DWIN_Draw_Rectangle(0, GetColor(eeprom_settings.highlight_box, Color_White), 145, 246, 254, 345);
      DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 181, 317, F("Info"));
    }
    else {
      DWIN_ICON_Show(ICON, ICON_Info_0, 145, 246);
      DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 181, 317, F("Info"));
    }
  #endif
}

void CrealityDWINClass::Draw_Main_Menu(uint8_t select/*=0*/) {
  process = Main;
  active_menu = MainMenu;
  selection = select;
  Clear_Screen();
  Draw_Title(Get_Menu_Title(MainMenu));
  SERIAL_ECHOPGM("\nDWIN handshake ");
  DWIN_ICON_Show(ICON, ICON_LOGO, 71, 72);
  Main_Menu_Icons();
}

void CrealityDWINClass::Print_Screen_Icons() {
  if (selection == 0) {
    DWIN_ICON_Show(ICON, ICON_Setup_1, 8, 252);
    DWIN_Draw_Rectangle(0, GetColor(eeprom_settings.highlight_box, Color_White), 8, 252, 87, 351);
    DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 30, 322, F("Tune"));
  }
  else {
    DWIN_ICON_Show(ICON, ICON_Setup_0, 8, 252);
    DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 30, 322, F("Tune"));
  }
  if (selection == 2) {
    DWIN_ICON_Show(ICON, ICON_Stop_1, 184, 252);
    DWIN_Draw_Rectangle(0, GetColor(eeprom_settings.highlight_box, Color_White), 184, 252, 263, 351);
    DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 205, 322, F("Stop"));
  }
  else {
    DWIN_ICON_Show(ICON, ICON_Stop_0, 184, 252);
    DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 205, 322, F("Stop"));
  }
  if (paused) {
    if (selection == 1) {
      DWIN_ICON_Show(ICON, ICON_Continue_1, 96, 252);
      DWIN_Draw_Rectangle(0, GetColor(eeprom_settings.highlight_box, Color_White), 96, 252, 175, 351);
      DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 114, 322, F("Resume"));
    }
    else {
      DWIN_ICON_Show(ICON, ICON_Continue_0, 96, 252);
      DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 114, 322, F("Resume"));
    }
  }
  else {
    if (selection == 1) {
      DWIN_ICON_Show(ICON, ICON_Pause_1, 96, 252);
      DWIN_Draw_Rectangle(0, GetColor(eeprom_settings.highlight_box, Color_White), 96, 252, 175, 351);
      DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 114, 322, F("Pause"));
    }
    else {
      DWIN_ICON_Show(ICON, ICON_Pause_0, 96, 252);
      DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Blue, 114, 322, F("Pause"));
    }
  }
}

void CrealityDWINClass::Draw_Print_Screen() {
  process = Print;
  selection = 0;
  Clear_Screen();
  // DWIN_Draw_Rectangle(1, Color_Bg_Black, 8, STATUS_Y, DWIN_WIDTH-8, 376);
  Draw_Title("Printing...");
  Print_Screen_Icons();
  DWIN_ICON_Show(ICON, ICON_PrintTime, 14, 171);
  DWIN_ICON_Show(ICON, ICON_RemainTime, 147, 169);
  DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, 41, 163, F("Elapsed"));
  DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, 176, 163, F("Remaining"));
  Update_Status_Bar(true);
  Draw_Print_ProgressBar();
  Draw_Print_ProgressElapsed();
  TERN_(USE_M73_REMAINING_TIME, Draw_Print_ProgressRemain());
  Draw_Print_Filename(true);
}

void CrealityDWINClass::Draw_Print_Filename(const bool reset/*=false*/) {
  static uint8_t namescrl = 0;
  if (reset) namescrl = 0;
  if (process == Print) {
    char dispname[STATUS_CHAR_LIMIT + 1];

    char *outstr = filename;
    size_t slen = strlen(filename);
    int8_t outlen = slen;
    if (slen > STATUS_CHAR_LIMIT) {
      int8_t pos = slen - namescrl, len = STATUS_CHAR_LIMIT;
      if (pos >= 0) {
        NOMORE(len, pos);
        LOOP_L_N(i, len) dispname[i] = filename[i + namescrl];
      }
      else {
        const int8_t mp = STATUS_CHAR_LIMIT + pos;
        LOOP_L_N(i, mp) dispname[i] = ' ';
        LOOP_S_L_N(i, mp, STATUS_CHAR_LIMIT) dispname[i] = filename[i - mp];
        if (mp <= 0) namescrl = 0;
      }
      dispname[len] = '\0';
      outstr = dispname;
      outlen = STATUS_CHAR_LIMIT;
      namescrl++;
    }
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 8, 50, DWIN_WIDTH - 8, 80);
    const int8_t npos = (DWIN_WIDTH - outlen * MENU_CHR_W) / 2;
    DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, npos, 60, outstr);
  }
}

void CrealityDWINClass::Draw_Print_ProgressBar() {
  uint8_t printpercent = sdprint ? card.percentDone() : (ui._get_progress() / 100);
  DWIN_ICON_Show(ICON, ICON_Bar, 15, 93);
  DWIN_Draw_Rectangle(1, BarFill_Color, 16 + printpercent * 240 / 100, 93, 256, 113);
  DWIN_Draw_IntValue(true, true, 0, DWIN_FONT_MENU, GetColor(eeprom_settings.progress_percent, Percent_Color), Color_Bg_Black, 3, 109, 133, printpercent);
  DWIN_Draw_String(false, DWIN_FONT_MENU, GetColor(eeprom_settings.progress_percent, Percent_Color), Color_Bg_Black, 133, 133, F("%"));
}

#if ENABLED(USE_M73_REMAINING_TIME)

  void CrealityDWINClass::Draw_Print_ProgressRemain() {
    uint16_t remainingtime = ui.get_remaining_time();
    DWIN_Draw_IntValue(true, true, 1, DWIN_FONT_MENU, GetColor(eeprom_settings.progress_time, Color_White), Color_Bg_Black, 2, 176, 187, remainingtime / 3600);
    DWIN_Draw_IntValue(true, true, 1, DWIN_FONT_MENU, GetColor(eeprom_settings.progress_time, Color_White), Color_Bg_Black, 2, 200, 187, (remainingtime % 3600) / 60);
    if (eeprom_settings.time_format_textual) {
      DWIN_Draw_String(false, DWIN_FONT_MENU, GetColor(eeprom_settings.progress_time, Color_White), Color_Bg_Black, 192, 187, F("h"));
      DWIN_Draw_String(false, DWIN_FONT_MENU, GetColor(eeprom_settings.progress_time, Color_White), Color_Bg_Black, 216, 187, F("m"));
    }
    else
      DWIN_Draw_String(false, DWIN_FONT_MENU, GetColor(eeprom_settings.progress_time, Color_White), Color_Bg_Black, 192, 187, F(":"));
  }

#endif

void CrealityDWINClass::Draw_Print_ProgressElapsed() {
  duration_t elapsed = print_job_timer.duration();
  DWIN_Draw_IntValue(true, true, 1, DWIN_FONT_MENU, GetColor(eeprom_settings.progress_time, Color_White), Color_Bg_Black, 2, 42, 187, elapsed.value / 3600);
  DWIN_Draw_IntValue(true, true, 1, DWIN_FONT_MENU, GetColor(eeprom_settings.progress_time, Color_White), Color_Bg_Black, 2, 66, 187, (elapsed.value % 3600) / 60);
  if (eeprom_settings.time_format_textual) {
    DWIN_Draw_String(false, DWIN_FONT_MENU, GetColor(eeprom_settings.progress_time, Color_White), Color_Bg_Black, 58, 187, F("h"));
    DWIN_Draw_String(false, DWIN_FONT_MENU, GetColor(eeprom_settings.progress_time, Color_White), Color_Bg_Black, 82, 187, F("m"));
  }
  else
    DWIN_Draw_String(false, DWIN_FONT_MENU, GetColor(eeprom_settings.progress_time, Color_White), Color_Bg_Black, 58, 187, F(":"));
}

void CrealityDWINClass::Draw_Print_confirm() {
  Draw_Print_Screen();
  Confirm_Handler(Complete);

/*
  process = Confirm;
  popup = Complete;
  DWIN_Draw_Rectangle(1, Color_Bg_Black, 8, 252, 263, 351);
  DWIN_ICON_Show(ICON, ICON_Confirm_E, 87, 283);
  DWIN_Draw_Rectangle(0, GetColor(eeprom_settings.highlight_box, Color_White), 86, 282, 187, 321);
  DWIN_Draw_Rectangle(0, GetColor(eeprom_settings.highlight_box, Color_White), 85, 281, 188, 322);
*/
}

void CrealityDWINClass::Draw_SD_Item(uint8_t item, uint8_t row, bool onlyCachedFileIcon/*=false*/) {
  if (item == 0)
    Draw_Menu_Item(0, ICON_Back, card.flag.workDirIsRoot ? F("Back") : F(".."));
  else {
    card.getfilename_sorted(SD_ORDER(item - 1, card.get_num_Files()));
    char * const filename = card.longest_filename();
    size_t max = MENU_CHAR_LIMIT;
    size_t pos = strlen(filename), len = pos;
    if (!card.flag.filenameIsDir)
      while (pos && filename[pos] != '.') pos--;
    len = pos;
    if (len > max) len = max;
    char name[len + 1];
    LOOP_L_N(i, len) name[i] = filename[i];
    if (pos > max)
      LOOP_S_L_N(i, len - 3, len) name[i] = '.';
    name[len] = '\0';
    Draw_Menu_Item(row, card.flag.filenameIsDir ? ICON_More : ICON_File, name, NULL, NULL, false, onlyCachedFileIcon);
  }
}

void CrealityDWINClass::Draw_SD_List(bool removed/*=false*/, uint8_t select/*=0*/, uint8_t scroll/*=0*/, bool onlyCachedFileIcon/*=false*/) {
  Clear_Screen();
  Draw_Title("Select File");
  // selection = 0;
  // scrollpos = 0;
  process = File;
  selection = min((int)select, card.get_num_Files()+1);
  scrollpos = scroll;
  if (selection-scrollpos > MROWS)
    scrollpos = selection - MROWS;
  if (card.isMounted() && !removed) {
    LOOP_L_N(i, _MIN(card.get_num_Files() + 1, TROWS))
      Draw_SD_Item(i+scrollpos, i, onlyCachedFileIcon);
  }
  else {
    Draw_Menu_Item(0, ICON_Back, F("Back"));
    DWIN_Draw_Rectangle(1, Color_Bg_Red, 10, MBASE(3) - 10, DWIN_WIDTH - 10, MBASE(4));
    DWIN_Draw_String(false, font16x32, Color_Yellow, Color_Bg_Red, ((DWIN_WIDTH) - 8 * 16) / 2, MBASE(3), F("No Media"));
  }
  DWIN_Draw_Rectangle(1, GetColor(eeprom_settings.cursor_color, Rectangle_Color), 0, MBASE(0) - 18, 14, MBASE(0) + 33);
}

void CrealityDWINClass::Draw_Status_Area(bool icons/*=false*/) {

  if (icons) DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, STATUS_Y, DWIN_WIDTH, DWIN_HEIGHT - 1);

  #define STATUS_X1 10
  #define STATUS_X2 112
  #define STATUS_X3 187
  #define STATUS_Y1 383
  #define STATUS_Y2 417

  int16_t htX = STATUS_X1, htY = STATUS_Y1;		// Hotend temp
  int16_t btX = STATUS_X1, btY = STATUS_Y2;   // Bed temp
  int16_t frX = STATUS_X2, frY = STATUS_Y1;		// feed rate
  int16_t flX = STATUS_X2, flY = STATUS_Y2;		// flow speed
  int16_t faX = STATUS_X3, faY = STATUS_Y1;		// Fan
  int16_t zoX = STATUS_X3, zoY = STATUS_Y2;		// Z-Offset

  #ifdef DEBUG_LCD_UI
    if (eeprom_settings.show_debug_on_LCD) {
      htX = STATUS_X2, htY = STATUS_Y2;		// Hotend temp
      frX = flX = faX = 0;

      if ( DEBUG_INFOLINE(1)){
        DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, STATUS_Y1, DWIN_WIDTH, STATUS_Y2);

        DWIN_Draw_String(false, font6x12, Color_White, Color_Bg_Black, 0, STATUS_Y1, dbg_InfoLine1);
        DWIN_Draw_String(false, font6x12, Color_White, Color_Bg_Black, 0, STATUS_Y1+13, dbg_InfoLine2);
      }
    }
  #endif

  #if HAS_HOTEND
    static float hotend = -1;
    static int16_t hotendtarget = -1, flow = -1;
    if (htX) {
      if (icons) {
        hotend = -1;
        hotendtarget = -1;
        DWIN_ICON_Show(ICON, ICON_HotendTemp, htX, htY);
        DWIN_Draw_String(false, DWIN_FONT_STAT, GetColor(eeprom_settings.status_area_text, Color_White), Color_Bg_Black, htX+15 + 3 * STAT_CHR_W + 5, htY+1, F("/"));
      }
      if (thermalManager.temp_hotend[0].celsius != hotend) {
        hotend = thermalManager.temp_hotend[0].celsius;
        DWIN_Draw_IntValue(true, true, 0, DWIN_FONT_STAT, GetColor(eeprom_settings.status_area_text, Color_White), Color_Bg_Black, 3, htX+13, htY+1, thermalManager.temp_hotend[0].celsius);
        DWIN_Draw_DegreeSymbol(GetColor(eeprom_settings.status_area_text, Color_White), htX+15 + 3 * STAT_CHR_W + 5, htY+3);
      }
      if (thermalManager.temp_hotend[0].target != hotendtarget) {
        hotendtarget = thermalManager.temp_hotend[0].target;
        DWIN_Draw_IntValue(true, true, 0, DWIN_FONT_STAT, GetColor(eeprom_settings.status_area_text, Color_White), Color_Bg_Black, 3, htX+15 + 4 * STAT_CHR_W + 6, htY+1, thermalManager.temp_hotend[0].target);
        DWIN_Draw_DegreeSymbol(GetColor(eeprom_settings.status_area_text, Color_White), htX+15 + 4 * STAT_CHR_W + 39, htY+3);
      }
    }
    if(flX){
      if (icons) {
        flow = -1;
        DWIN_ICON_Show(ICON, ICON_StepE, flX, flY);
        DWIN_Draw_String(false, DWIN_FONT_STAT, GetColor(eeprom_settings.status_area_text, Color_White), Color_Bg_Black, flX+4 + 5 * STAT_CHR_W + 2, flY, F("%"));
      }
      if (planner.flow_percentage[0] != flow) {
        flow = planner.flow_percentage[0];
        DWIN_Draw_IntValue(true, true, 0, DWIN_FONT_STAT, GetColor(eeprom_settings.status_area_text, Color_White), Color_Bg_Black, 3, flX+4 + 2 * STAT_CHR_W, flY, planner.flow_percentage[0]);
      }
    }
  #endif

  #if HAS_HEATED_BED
    static float bed = -1;
    static int16_t bedtarget = -1;
    if (btX) {
      if (icons) {
        bed = -1;
        bedtarget = -1;
        DWIN_ICON_Show(ICON, ICON_BedTemp, btX, btY-1);
        DWIN_Draw_String(false, DWIN_FONT_STAT, GetColor(eeprom_settings.status_area_text, Color_White), Color_Bg_Black, btX+15 + 3 * STAT_CHR_W + 5, btY, F("/"));
      }
      if (thermalManager.temp_bed.celsius != bed) {
        bed = thermalManager.temp_bed.celsius;
        DWIN_Draw_IntValue(true, true, 0, DWIN_FONT_STAT, GetColor(eeprom_settings.status_area_text, Color_White), Color_Bg_Black, 3, btX+18, btY, thermalManager.temp_bed.celsius);
        DWIN_Draw_DegreeSymbol(GetColor(eeprom_settings.status_area_text, Color_White), btX+15 + 3 * STAT_CHR_W + 5, btY+2);
      }
      if (thermalManager.temp_bed.target != bedtarget) {
        bedtarget = thermalManager.temp_bed.target;
        DWIN_Draw_IntValue(true, true, 0, DWIN_FONT_STAT, GetColor(eeprom_settings.status_area_text, Color_White), Color_Bg_Black, 3, btX+15 + 4 * STAT_CHR_W + 6, btY, thermalManager.temp_bed.target);
        DWIN_Draw_DegreeSymbol(GetColor(eeprom_settings.status_area_text, Color_White), btX+15 + 4 * STAT_CHR_W + 39, btY+2);
      }
    }
  #endif

  #if HAS_FAN
    static uint8_t fan = -1;
    if (faX) {
      if (icons) {
        fan = -1;
        DWIN_ICON_Show(ICON, ICON_FanSpeed, faX, faY);
      }
      if (thermalManager.fan_speed[0] != fan) {
        fan = thermalManager.fan_speed[0];
        DWIN_Draw_IntValue(true, true, 0, DWIN_FONT_STAT, GetColor(eeprom_settings.status_area_text, Color_White), Color_Bg_Black, 3, faX+8 + 2 * STAT_CHR_W, faY+1, thermalManager.fan_speed[0]);
      }
    }
  #endif

  #if HAS_ZOFFSET_ITEM
    static float offset = -1;
    if (zoX) {
      if (icons) {
        offset = -1;
        DWIN_ICON_Show(ICON, ICON_Zoffset, zoX, zoY-1);
      }
      if (zoffsetvalue != offset) {
        offset = zoffsetvalue;
        DWIN_Draw_FloatValue(true, true, 0, DWIN_FONT_STAT, GetColor(eeprom_settings.status_area_text, Color_White), Color_Bg_Black, 2, 2, zoX+20, zoY, (zoffsetvalue < 0 ? -zoffsetvalue : zoffsetvalue));
        DWIN_Draw_String(true, DWIN_FONT_MENU, GetColor(eeprom_settings.status_area_text, Color_White), Color_Bg_Black, zoX+18, zoY+2, zoffsetvalue < 0 ? F("-") : F(" "));
      }
    }
  #endif

  static int16_t feedrate = -1;
  if (icons) {
    if (frX) {
      feedrate = -1;
      DWIN_ICON_Show(ICON, ICON_Speed, frX+1, frY);
      DWIN_Draw_String(false, DWIN_FONT_STAT, GetColor(eeprom_settings.status_area_text, Color_White), Color_Bg_Black, frX+4 + 5 * STAT_CHR_W + 2, frY+1, F("%"));
    }
    if (feedrate_percentage != feedrate) {
      feedrate = feedrate_percentage;
      DWIN_Draw_IntValue(true, true, 0, DWIN_FONT_STAT, GetColor(eeprom_settings.status_area_text, Color_White), Color_Bg_Black, 3, frX+4 + 2 * STAT_CHR_W, frY+1, feedrate_percentage);
    }
  }

  static float x = -1, y = -1, z = -1;
  static bool update_x = false, update_y = false, update_z = false;
  update_x = (current_position.x != x || axis_should_home(X_AXIS) || update_x);
  update_y = (current_position.y != y || axis_should_home(Y_AXIS) || update_y);
  update_z = (current_position.z != z || axis_should_home(Z_AXIS) || update_z);
  if (icons) {
    x = y = z = -1;
    DWIN_Draw_Line(GetColor(eeprom_settings.coordinates_split_line, Line_Color, true), 16, 450, 256, 450);
    DWIN_ICON_Show(ICON, ICON_MaxSpeedX,  10, 456);
    DWIN_ICON_Show(ICON, ICON_MaxSpeedY,  95, 456);
    DWIN_ICON_Show(ICON, ICON_MaxSpeedZ, 180, 456);
  }
  if (update_x) {
    x = current_position.x;
    if ((update_x = axis_should_home(X_AXIS) && ui.get_blink()))
      DWIN_Draw_String(true, DWIN_FONT_MENU, GetColor(eeprom_settings.coordinates_text, Color_White), Color_Bg_Black, 35, 459, F("  -?-  "));
    else
      DWIN_Draw_FloatValue(true, true, 0, DWIN_FONT_MENU, GetColor(eeprom_settings.coordinates_text, Color_White), Color_Bg_Black, 3, 1, 35, 459, current_position.x);
  }
  if (update_y) {
    y = current_position.y;
    if ((update_y = axis_should_home(Y_AXIS) && ui.get_blink()))
      DWIN_Draw_String(true, DWIN_FONT_MENU, GetColor(eeprom_settings.coordinates_text, Color_White), Color_Bg_Black, 120, 459, F("  -?-  "));
    else
      DWIN_Draw_FloatValue(true, true, 0, DWIN_FONT_MENU, GetColor(eeprom_settings.coordinates_text, Color_White), Color_Bg_Black, 3, 1, 120, 459, current_position.y);
  }
  if (update_z) {
    z = current_position.z;
    if ((update_z = axis_should_home(Z_AXIS) && ui.get_blink()))
      DWIN_Draw_String(true, DWIN_FONT_MENU, GetColor(eeprom_settings.coordinates_text, Color_White), Color_Bg_Black, 205, 459, F("  -?-  "));
    else
      DWIN_Draw_FloatValue(true, true, 0, DWIN_FONT_MENU, GetColor(eeprom_settings.coordinates_text, Color_White), Color_Bg_Black, 3, 2, 205, 459, (current_position.z>=0) ? current_position.z : 0);
  }
  DWIN_UpdateLCD();
}

void CrealityDWINClass::Draw_Popup(FSTR_P const line1, FSTR_P const line2, FSTR_P const line3, uint8_t mode, uint8_t icon/*=0*/) {
  if (process != Confirm && process != Popup && process != Wait) last_process = process;
  if ((process == Menu || process == Wait) && mode == Popup) last_selection = selection;
  process = mode;
  Clear_Screen();
  // must not overwrite status area due to recover
  DWIN_Draw_Rectangle(0, Color_White, 13, 59, 259, STATUS_Y-1);
  DWIN_Draw_Rectangle(1, Color_Bg_Window, 14, 60, 258, STATUS_Y-2);
  const uint8_t ypos = (mode == Popup || mode == Confirm) ? 150 : 230;
  if (icon > 0) DWIN_ICON_Show(ICON, icon, 101, 105);
  DWIN_Draw_String(true, DWIN_FONT_MENU, Popup_Text_Color, Color_Bg_Window, (272 - 8 * strlen_P(FTOP(line1))) / 2, ypos, line1);
  DWIN_Draw_String(true, DWIN_FONT_MENU, Popup_Text_Color, Color_Bg_Window, (272 - 8 * strlen_P(FTOP(line2))) / 2, ypos + 30, line2);
  DWIN_Draw_String(true, DWIN_FONT_MENU, Popup_Text_Color, Color_Bg_Window, (272 - 8 * strlen_P(FTOP(line3))) / 2, ypos + 60, line3);
  if (mode == Popup) {
    selection = 0;
    DWIN_Draw_Rectangle(1, Confirm_Color, 26, 280, 125, 317);
    DWIN_Draw_Rectangle(1, Cancel_Color, 146, 280, 245, 317);
    DWIN_Draw_String(false, DWIN_FONT_STAT, Color_White, Color_Bg_Window, 39, 290, F("Confirm"));
    DWIN_Draw_String(false, DWIN_FONT_STAT, Color_White, Color_Bg_Window, 165, 290, F("Cancel"));
    Popup_Select();
  }
  else if (mode == Confirm) {
    DWIN_Draw_Rectangle(1, Confirm_Color, 87, 280, 186, 317);
    DWIN_Draw_String(false, DWIN_FONT_STAT, Color_White, Color_Bg_Window, 96, 290, F("Continue"));
  }
}

void MarlinUI::kill_screen(FSTR_P const error, FSTR_P const) {
  CrealityDWIN.Draw_Popup(F("Printer Kill Reason:"), error, F("Restart Required"), Wait, ICON_BLTouch);
}

void CrealityDWINClass::Popup_Select() {
  const uint16_t c1 = (selection == 0) ? GetColor(eeprom_settings.highlight_box, Color_White) : Color_Bg_Window,
                 c2 = (selection == 0) ? Color_Bg_Window : GetColor(eeprom_settings.highlight_box, Color_White);
  DWIN_Draw_Rectangle(0, c1, 25, 279, 126, 318);
  DWIN_Draw_Rectangle(0, c1, 24, 278, 127, 319);
  DWIN_Draw_Rectangle(0, c2, 145, 279, 246, 318);
  DWIN_Draw_Rectangle(0, c2, 144, 278, 247, 319);
}

void CrealityDWINClass::Update_Status_Bar(bool refresh/*=false*/) {
  static bool new_msg;
  static uint8_t msgscrl = 0;
  static char lastmsg[64];
  if (strcmp_P(lastmsg, statusmsg) != 0 || refresh) {
    strcpy_P(lastmsg, statusmsg);
    msgscrl = 0;
    new_msg = true;
  }
  size_t len = strlen(statusmsg);
  int8_t pos = len;
  if (pos > STATUS_CHAR_LIMIT) {
    pos -= msgscrl;
    len = _MIN((size_t)pos, (size_t)STATUS_CHAR_LIMIT);
    char dispmsg[len + 1];
    if (pos >= 0) {
      LOOP_L_N(i, len) dispmsg[i] = statusmsg[i + msgscrl];
    }
    else {
      LOOP_L_N(i, STATUS_CHAR_LIMIT + pos) dispmsg[i] = ' ';
      LOOP_S_L_N(i, STATUS_CHAR_LIMIT + pos, STATUS_CHAR_LIMIT) dispmsg[i] = statusmsg[i - (STATUS_CHAR_LIMIT + pos)];
    }
    dispmsg[len] = '\0';
    if (false) { // (process == Print) {
      DWIN_Draw_Rectangle(1, Color_Grey, 8, 214, DWIN_WIDTH - 8, 238);
      const int8_t npos = (DWIN_WIDTH - STATUS_CHAR_LIMIT * MENU_CHR_W) / 2;
      DWIN_Draw_String(false, DWIN_FONT_MENU, GetColor(eeprom_settings.status_bar_text, Color_White), Color_Bg_Black, npos, 219, dispmsg);
    }
    else {
      DWIN_Draw_Rectangle(1, Color_Bg_Black, 8, STATUS_Y, DWIN_WIDTH-8, 376);
      const int8_t npos = (DWIN_WIDTH - STATUS_CHAR_LIMIT * MENU_CHR_W) / 2;
      DWIN_Draw_String(false, DWIN_FONT_MENU, GetColor(eeprom_settings.status_bar_text, Color_White), Color_Bg_Black, npos, 357, dispmsg);
    }
    if (-pos >= STATUS_CHAR_LIMIT) msgscrl = 0;
    msgscrl++;
  }
  else {
    if (new_msg) {
      new_msg = false;
      if (false) { // (process == Print) {
        DWIN_Draw_Rectangle(1, Color_Grey, 8, 214, DWIN_WIDTH - 8, 238);
        const int8_t npos = (DWIN_WIDTH - strlen(statusmsg) * MENU_CHR_W) / 2;
        DWIN_Draw_String(false, DWIN_FONT_MENU, GetColor(eeprom_settings.status_bar_text, Color_White), Color_Bg_Black, npos, 219, statusmsg);
      }
      else {
        DWIN_Draw_Rectangle(1, Color_Bg_Black, 8, STATUS_Y, DWIN_WIDTH-8, 376);
        const int8_t npos = (DWIN_WIDTH - strlen(statusmsg) * MENU_CHR_W) / 2;
        DWIN_Draw_String(false, DWIN_FONT_MENU, GetColor(eeprom_settings.status_bar_text, Color_White), Color_Bg_Black, npos, 357, statusmsg);
      }
    }
  }
}

void CrealityDWINClass::Draw_Keyboard(bool restrict, bool numeric, uint8_t selected, bool uppercase/*=false*/, bool lock/*=false*/) {
  process = Keyboard;
  keyboard_restrict = restrict;
  numeric_keyboard = numeric;
  DWIN_Draw_Rectangle(0, Color_White, 0, KEY_Y_START, DWIN_WIDTH-2, DWIN_HEIGHT-2);
  DWIN_Draw_Rectangle(1, Color_Bg_Black, 1, KEY_Y_START+1, DWIN_WIDTH-3, DWIN_HEIGHT-3);
  LOOP_L_N(i, 36) Draw_Keys(i, (i == selected), uppercase, lock);
}

void CrealityDWINClass::Draw_Keys(uint8_t index, bool selected, bool uppercase/*=false*/, bool lock/*=false*/) {
  const char *keys;
  if (numeric_keyboard) keys = "1234567890&<>(){}[]*\"\':;!?";
  else keys = (uppercase) ? "QWERTYUIOPASDFGHJKLZXCVBNM" : "qwertyuiopasdfghjklzxcvbnm";
  #define KEY_X1(x) x*KEY_WIDTH+KEY_INSET+KEY_PADDING
  #define KEY_X2(x) (x+1)*KEY_WIDTH+KEY_INSET-KEY_PADDING
  #define KEY_Y1(y) KEY_Y_START+KEY_INSET+KEY_PADDING+y*KEY_HEIGHT
  #define KEY_Y2(y) KEY_Y_START+KEY_INSET-KEY_PADDING+(y+1)*KEY_HEIGHT

  const uint8_t rowCount[3] = {10, 9, 7};
  const float xOffset[3] = {0, 0.5f*KEY_WIDTH, 1.5f*KEY_WIDTH};

  if (index < 28) {
    if (index == 19) {
      DWIN_Draw_Rectangle(0, Color_Light_Blue, KEY_X1(0), KEY_Y1(2), KEY_X2(0)+xOffset[1], KEY_Y2(2));
      DWIN_Draw_Rectangle(0, (selected) ? Select_Color : Color_Bg_Black, KEY_X1(0)+1, KEY_Y1(2)+1, KEY_X2(0)+xOffset[1]-1, KEY_Y2(2)-1);
      if (!numeric_keyboard) {
        if (lock) {
          DWIN_Draw_Line(Select_Color, KEY_X1(0)+17, KEY_Y1(2)+16, KEY_X1(0)+25, KEY_Y1(2)+8);
          DWIN_Draw_Line(Select_Color, KEY_X1(0)+17, KEY_Y1(2)+16, KEY_X1(0)+9, KEY_Y1(2)+8);
        }
        else {
          DWIN_Draw_Line((uppercase) ? Select_Color : Color_White, KEY_X1(0)+17, KEY_Y1(2)+8, KEY_X1(0)+25, KEY_Y1(2)+16);
          DWIN_Draw_Line((uppercase) ? Select_Color : Color_White, KEY_X1(0)+17, KEY_Y1(2)+8, KEY_X1(0)+9, KEY_Y1(2)+16);
        }
      }
    }
    else if (index == 27) {
      DWIN_Draw_Rectangle(0, Color_Light_Blue, KEY_X1(7)+xOffset[2], KEY_Y1(2), KEY_X2(9), KEY_Y2(2));
      DWIN_Draw_Rectangle(0, (selected) ? Select_Color : Color_Bg_Black, KEY_X1(7)+xOffset[2]+1, KEY_Y1(2)+1, KEY_X2(9)-1, KEY_Y2(2)-1);
      DWIN_Draw_String(true, DWIN_FONT_MENU, Color_Red, Color_Bg_Black, KEY_X1(7)+xOffset[2]+3, KEY_Y1(2)+5, F("<--"));
    }
    else {
      if (index > 19) index--;
      if (index > 27) index--;
      uint8_t y, x;
      if (index < rowCount[0]) y = 0, x = index;
      else if (index < (rowCount[0]+rowCount[1])) y = 1, x = index-rowCount[0];
      else y = 2, x = index-(rowCount[0]+rowCount[1]);
      const char keyStr[2] = {keys[(y>0)*rowCount[0]+(y>1)*rowCount[1]+x], '\0'};
      DWIN_Draw_Rectangle(0, Color_Light_Blue, KEY_X1(x)+xOffset[y], KEY_Y1(y), KEY_X2(x)+xOffset[y], KEY_Y2(y));
      DWIN_Draw_Rectangle(0, (selected) ? Select_Color : Color_Bg_Black, KEY_X1(x)+xOffset[y]+1, KEY_Y1(y)+1, KEY_X2(x)+xOffset[y]-1, KEY_Y2(y)-1);
      DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, KEY_X1(x)+xOffset[y]+5, KEY_Y1(y)+5, keyStr);
      if (keyboard_restrict && numeric_keyboard && index > 9) {
        DWIN_Draw_Line(Color_Light_Red, KEY_X1(x)+xOffset[y]+1, KEY_Y1(y)+1, KEY_X2(x)+xOffset[y]-1, KEY_Y2(y)-1);
        DWIN_Draw_Line(Color_Light_Red, KEY_X1(x)+xOffset[y]+1, KEY_Y2(y)-1, KEY_X2(x)+xOffset[y]-1, KEY_Y1(y)+1);
      }
    }
  }
  else {
    switch (index) {
      case 28:
        DWIN_Draw_Rectangle(0, Color_Light_Blue, KEY_X1(0), KEY_Y1(3), KEY_X2(0)+xOffset[1], KEY_Y2(3));
        DWIN_Draw_Rectangle(0, (selected) ? Select_Color : Color_Bg_Black, KEY_X1(0)+1, KEY_Y1(3)+1, KEY_X2(0)+xOffset[1]-1, KEY_Y2(3)-1);
        DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, KEY_X1(0)-1, KEY_Y1(3)+5, F("?123"));
        break;
      case 29:
        DWIN_Draw_Rectangle(0, Color_Light_Blue, KEY_X1(1)+xOffset[1], KEY_Y1(3), KEY_X2(1)+xOffset[1], KEY_Y2(3));
        DWIN_Draw_Rectangle(0, (selected) ? Select_Color : Color_Bg_Black, KEY_X1(1)+xOffset[1]+1, KEY_Y1(3)+1, KEY_X2(1)+xOffset[1]-1, KEY_Y2(3)-1);
        DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, KEY_X1(1)+xOffset[1]+5, KEY_Y1(3)+5, F("-"));
        break;
      case 30:
        DWIN_Draw_Rectangle(0, Color_Light_Blue, KEY_X1(2)+xOffset[1], KEY_Y1(3), KEY_X2(2)+xOffset[1], KEY_Y2(3));
        DWIN_Draw_Rectangle(0, (selected) ? Select_Color : Color_Bg_Black, KEY_X1(2)+xOffset[1]+1, KEY_Y1(3)+1, KEY_X2(2)+xOffset[1]-1, KEY_Y2(3)-1);
        DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, KEY_X1(2)+xOffset[1]+5, KEY_Y1(3)+5, F("_"));
        break;
      case 31:
        DWIN_Draw_Rectangle(0, Color_Light_Blue, KEY_X1(3)+xOffset[1], KEY_Y1(3), KEY_X2(5)+xOffset[1], KEY_Y2(3));
        DWIN_Draw_Rectangle(0, (selected) ? Select_Color : Color_Bg_Black, KEY_X1(3)+xOffset[1]+1, KEY_Y1(3)+1, KEY_X2(5)+xOffset[1]-1, KEY_Y2(3)-1);
        DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, KEY_X1(3)+xOffset[1]+14, KEY_Y1(3)+5, F("Space"));
        if (keyboard_restrict) {
          DWIN_Draw_Line(Color_Light_Red, KEY_X1(3)+xOffset[1]+1, KEY_Y1(3)+1, KEY_X2(5)+xOffset[1]-1, KEY_Y2(3)-1);
          DWIN_Draw_Line(Color_Light_Red, KEY_X1(3)+xOffset[1]+1, KEY_Y2(3)-1, KEY_X2(5)+xOffset[1]-1, KEY_Y1(3)+1);
        }
        break;
      case 32:
        DWIN_Draw_Rectangle(0, Color_Light_Blue, KEY_X1(6)+xOffset[1], KEY_Y1(3), KEY_X2(6)+xOffset[1], KEY_Y2(3));
        DWIN_Draw_Rectangle(0, (selected) ? Select_Color : Color_Bg_Black, KEY_X1(6)+xOffset[1]+1, KEY_Y1(3)+1, KEY_X2(6)+xOffset[1]-1, KEY_Y2(3)-1);
        DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, KEY_X1(6)+xOffset[1]+7, KEY_Y1(3)+5, F("."));
        if (keyboard_restrict) {
          DWIN_Draw_Line(Color_Light_Red, KEY_X1(6)+xOffset[1]+1, KEY_Y1(3)+1, KEY_X2(6)+xOffset[1]-1, KEY_Y2(3)-1);
          DWIN_Draw_Line(Color_Light_Red, KEY_X1(6)+xOffset[1]+1, KEY_Y2(3)-1, KEY_X2(6)+xOffset[1]-1, KEY_Y1(3)+1);
        }
        break;
      case 33:
        DWIN_Draw_Rectangle(0, Color_Light_Blue, KEY_X1(7)+xOffset[1], KEY_Y1(3), KEY_X2(7)+xOffset[1], KEY_Y2(3));
        DWIN_Draw_Rectangle(0, (selected) ? Select_Color : Color_Bg_Black, KEY_X1(7)+xOffset[1]+1, KEY_Y1(3)+1, KEY_X2(7)+xOffset[1]-1, KEY_Y2(3)-1);
        DWIN_Draw_String(false, DWIN_FONT_MENU, Color_White, Color_Bg_Black, KEY_X1(7)+xOffset[1]+4, KEY_Y1(3)+5, F("/"));
        if (keyboard_restrict) {
          DWIN_Draw_Line(Color_Light_Red, KEY_X1(7)+xOffset[1]+1, KEY_Y1(3)+1, KEY_X2(7)+xOffset[1]-1, KEY_Y2(3)-1);
          DWIN_Draw_Line(Color_Light_Red, KEY_X1(7)+xOffset[1]+1, KEY_Y2(3)-1, KEY_X2(7)+xOffset[1]-1, KEY_Y1(3)+1);
        }
        break;
      case 34:
        DWIN_Draw_Rectangle(0, Color_Light_Blue, KEY_X1(7)+xOffset[2], KEY_Y1(3), KEY_X2(9), KEY_Y2(3));
        DWIN_Draw_Rectangle(0, (selected) ? Select_Color : Color_Bg_Black, KEY_X1(7)+xOffset[2]+1, KEY_Y1(3)+1, KEY_X2(9)-1, KEY_Y2(3)-1);
        DWIN_Draw_String(true, DWIN_FONT_MENU, Color_Cyan, Color_Bg_Black, KEY_X1(7)+xOffset[2]+3, KEY_Y1(3)+5, F("-->"));
        break;
    }
  }
}

#include "dwin#2_menu.cpp.h"

/* Popup Config */

void CrealityDWINClass::Popup_Handler(PopupID popupid, bool option/*=false*/) {
  popup = last_popup = popupid;

  DEBUG_ECHOLNPGM("CrealityDWINClass::Popup_Handler (popupid=", popupid, ", option=", option, ")");

  switch (popupid) {
    case Pause:         Draw_Popup(F("Pause Print"), F(""), F(""), Popup); break;
    case Stop:          Draw_Popup(F("Stop Print"), F(""), F(""), Popup); break;
    case Resume:        Draw_Popup(F("Resume Print?"), F("Looks Like the last"), F("print was interrupted."), Popup); break;
    case ConfFilChange: Draw_Popup(F("Confirm Filament Change"), F(""), F(""), Popup); break;
    case PurgeMore:     Draw_Popup(F("Purge more filament?"), F("(Cancel to finish process)"), F(""), Popup); break;
    case SaveLevel:     Draw_Popup(F("Leveling Complete"), F("Save to EEPROM?"), F(""), Popup); break;
    case MeshSlot:      Draw_Popup(F("Mesh slot not selected"), F("(Confirm to select slot 0)"), F(""), Popup); break;
    case ETemp:         Draw_Popup(F("Nozzle is too cold"), F("Open Preheat Menu?"), F(""), Popup); break;
    case ManualProbing: Draw_Popup(F("Manual Probing"), F("(Confirm to probe)"), F("(cancel to exit)"), Popup); break;
    case Level:         Draw_Popup(F("Auto Bed Leveling"), F("Please wait until done."), F(""), Wait, ICON_AutoLeveling); break;
    case Home:          Draw_Popup(option ? F("Parking") : F("Homing"), F("Please wait until done."), F(""), Wait, ICON_BLTouch); break;
    case MoveWait:      Draw_Popup(F("Moving to Point"), F("Please wait until done."), F(""), Wait, ICON_BLTouch); break;
    case Heating:       Draw_Popup(F("Heating"), F("Please wait until done."), F(""), Wait, ICON_BLTouch); break;
    case FilLoad:       Draw_Popup(option ? F("Unloading Filament") : F("Loading Filament"), F("Please wait until done."), F(""), Wait, ICON_BLTouch); break;
    case FilChange:     Draw_Popup(F("Filament Change"), F("Please wait for prompt."), F(""), Wait, ICON_BLTouch); break;
    case TempWarn:      Draw_Popup(option ? F("Nozzle temp too low!") : F("Nozzle temp too high!"), F(""), F(""), Wait, option ? ICON_TempTooLow : ICON_TempTooHigh); break;
    case Runout:        Draw_Popup(F("Filament Runout"), F(""), F(""), Wait, ICON_BLTouch); break;
    case PIDWait:       Draw_Popup(F("PID Autotune"), F("in process"), F("Please wait until done."), Wait, ICON_BLTouch); break;
    case Resuming:      Draw_Popup(F("Resuming Print"), F("Please wait until done."), F(""), Wait, ICON_BLTouch); break;
    case ConfirmStartPrint: Draw_Popup(option ? F("Loading Preview...") : F("Print file?"), F(""), F(""), Popup); break;
    default: break;
  }
}

void CrealityDWINClass::Confirm_Handler(PopupID popupid) {

  DEBUG_ECHOLNPGM("CrealityDWINClass::Confirm_Handler (popupid=", popupid, ")");

  popup = popupid;
  switch (popupid) {
    case FilInsert:   Draw_Popup(F("Insert Filament"), F("Press to Continue"), F(""), Confirm); break;
    case HeaterTime:  Draw_Popup(F("Heater Timed Out"), F("Press to Reheat"), F(""), Confirm); break;
    case UserInput:   Draw_Popup(F("Waiting for Input"), F("Press to Continue"), F(""), Confirm); break;
    case LevelError:  Draw_Popup(F("Couldn't enable Leveling"), F("(Valid mesh must exist)"), F(""), Confirm); break;
    case InvalidMesh: Draw_Popup(F("Valid mesh must exist"), F("before tuning can be"), F("performed"), Confirm); break;
    case Complete:    Draw_Popup(F("Printing finished !!"), F(""), F(""), Confirm); break;
  default: break;
  }
}

/* Navigation and Control */

void CrealityDWINClass::Main_Menu_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW && selection < PAGE_COUNT - 1) {
    selection++; // Select Down
    Main_Menu_Icons();
  }
  else if (encoder_diffState == ENCODER_DIFF_CCW && selection > 0) {
    selection--; // Select Up
    Main_Menu_Icons();
  }
  else if (encoder_diffState == ENCODER_DIFF_ENTER)
    switch (selection) {
      case PAGE_PRINT: card.mount(); Draw_SD_List(); break;
      case PAGE_PREPARE: Draw_Menu(Prepare); break;
      case PAGE_CONTROL: Draw_Menu(Control); break;
      case PAGE_INFO_LEVELING: Draw_Menu(TERN(HAS_MESH, Leveling, InfoMain)); break;
    }
  DWIN_UpdateLCD();
}

void CrealityDWINClass::Menu_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW && selection < Get_Menu_Size(active_menu)) {
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
    selection++; // Select Down
    if (selection > scrollpos+MROWS) {
      scrollpos++;
      DWIN_Frame_AreaMove(1, 2, MLINE, Color_Bg_Black, 0, 31, DWIN_WIDTH, 349);
      Menu_Item_Handler(active_menu, selection);
    }
    DWIN_Draw_Rectangle(1, GetColor(eeprom_settings.cursor_color, Rectangle_Color), 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
  }
  else if (encoder_diffState == ENCODER_DIFF_CCW && selection > 0) {
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
    selection--; // Select Up
    if (selection < scrollpos) {
      scrollpos--;
      DWIN_Frame_AreaMove(1, 3, MLINE, Color_Bg_Black, 0, 31, DWIN_WIDTH, 349);
      Menu_Item_Handler(active_menu, selection);
    }
    DWIN_Draw_Rectangle(1, GetColor(eeprom_settings.cursor_color, Rectangle_Color), 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
  }
  else if (encoder_diffState == ENCODER_DIFF_ENTER)
    Menu_Item_Handler(active_menu, selection, false);
  DWIN_UpdateLCD();
}

void CrealityDWINClass::Value_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW)
    tempvalue += EncoderRate.encoderMoveValue;
  else if (encoder_diffState == ENCODER_DIFF_CCW)
    tempvalue -= EncoderRate.encoderMoveValue;
  else if (encoder_diffState == ENCODER_DIFF_ENTER) {
    process = Menu;
    EncoderRate.enabled = false;
    Draw_Float(tempvalue / valueunit, selection - scrollpos, false, valueunit);
    DWIN_UpdateLCD();
    if (active_menu == ZOffset && liveadjust) {
      planner.synchronize();
      current_position.z += (tempvalue / valueunit - zoffsetvalue);
      planner.buffer_line(current_position, homing_feedrate(Z_AXIS), active_extruder);
      current_position.z = 0;
      sync_plan_position();
    }
    else if (active_menu == Tune && selection == TUNE_ZOFFSET) {
      sprintf_P(cmd, PSTR("M290 Z%s"), dtostrf((tempvalue / valueunit - zoffsetvalue), 1, 3, str_1));
      gcode.process_subcommands_now(cmd);
    }
    if (TERN0(HAS_HOTEND, valuepointer == &thermalManager.temp_hotend[0].pid.Ki) || TERN0(HAS_HEATED_BED, valuepointer == &thermalManager.temp_bed.pid.Ki))
      tempvalue = scalePID_i(tempvalue);
    if (TERN0(HAS_HOTEND, valuepointer == &thermalManager.temp_hotend[0].pid.Kd) || TERN0(HAS_HEATED_BED, valuepointer == &thermalManager.temp_bed.pid.Kd))
      tempvalue = scalePID_d(tempvalue);
    switch (valuetype) {
      case 0: *(float*)valuepointer = tempvalue / valueunit; break;
      case 1: *(uint8_t*)valuepointer = tempvalue / valueunit; break;
      case 2: *(uint16_t*)valuepointer = tempvalue / valueunit; break;
      case 3: *(int16_t*)valuepointer = tempvalue / valueunit; break;
      case 4: *(uint32_t*)valuepointer = tempvalue / valueunit; break;
      case 5: *(int8_t*)valuepointer = tempvalue / valueunit; break;
    }
    switch (active_menu) {
      case Move:
        planner.synchronize();
        planner.buffer_line(current_position, manual_feedrate_mm_s[selection - 1], active_extruder);
        break;
      #if HAS_MESH
        case ManualMesh:
          planner.synchronize();
          planner.buffer_line(current_position, homing_feedrate(Z_AXIS), active_extruder);
          planner.synchronize();
          break;
        case UBLMesh:     mesh_conf.manual_move(true); break;
        case LevelManual: mesh_conf.manual_move(selection == LEVELING_M_OFFSET); break;
      #endif
    }
    if (valuepointer == &planner.flow_percentage[0])
      planner.refresh_e_factor(0);
    if (funcpointer) funcpointer();
    return;
  }
  NOLESS(tempvalue, (valuemin * valueunit));
  NOMORE(tempvalue, (valuemax * valueunit));
  Draw_Float(tempvalue / valueunit, selection - scrollpos, true, valueunit);
  DWIN_UpdateLCD();
  if (active_menu == Move && livemove) {
    *(float*)valuepointer = tempvalue / valueunit;
    planner.buffer_line(current_position, manual_feedrate_mm_s[selection - 1], active_extruder);
  }
}

void CrealityDWINClass::Option_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW)
    tempvalue += EncoderRate.encoderMoveValue;
  else if (encoder_diffState == ENCODER_DIFF_CCW)
    tempvalue -= EncoderRate.encoderMoveValue;
  else if (encoder_diffState == ENCODER_DIFF_ENTER) {
    process = Menu;
    EncoderRate.enabled = false;
    if (valuepointer == &color_names) {
      switch (selection) {
        case COLORSETTINGS_CURSOR: eeprom_settings.cursor_color = tempvalue; break;
        case COLORSETTINGS_SPLIT_LINE: eeprom_settings.menu_split_line = tempvalue; break;
        case COLORSETTINGS_MENU_TOP_BG: eeprom_settings.menu_top_bg = tempvalue; break;
        case COLORSETTINGS_MENU_TOP_TXT: eeprom_settings.menu_top_txt = tempvalue; break;
        case COLORSETTINGS_HIGHLIGHT_BORDER: eeprom_settings.highlight_box = tempvalue; break;
        case COLORSETTINGS_PROGRESS_PERCENT: eeprom_settings.progress_percent = tempvalue; break;
        case COLORSETTINGS_PROGRESS_TIME: eeprom_settings.progress_time = tempvalue; break;
        case COLORSETTINGS_PROGRESS_STATUS_BAR: eeprom_settings.status_bar_text = tempvalue; break;
        case COLORSETTINGS_PROGRESS_STATUS_AREA: eeprom_settings.status_area_text = tempvalue; break;
        case COLORSETTINGS_PROGRESS_COORDINATES: eeprom_settings.coordinates_text = tempvalue; break;
        case COLORSETTINGS_PROGRESS_COORDINATES_LINE: eeprom_settings.coordinates_split_line = tempvalue; break;
      }
      Redraw_Screen();
    }
    else if (valuepointer == &preheat_modes)
      preheatmode = tempvalue;

    Draw_Option(tempvalue, static_cast<const char * const *>(valuepointer), selection - scrollpos, false, (valuepointer == &color_names));
    DWIN_UpdateLCD();
    return;
  }
  NOLESS(tempvalue, valuemin);
  NOMORE(tempvalue, valuemax);
  Draw_Option(tempvalue, static_cast<const char * const *>(valuepointer), selection - scrollpos, true);
  DWIN_UpdateLCD();
}


#if ENABLED(DWIN_CREALITY_LCD_JYERSUI_GCODE_PREVIEW)
bool CrealityDWINClass::find_and_decode_gcode_preview(char *name, uint8_t preview_type, uint16_t *address, bool onlyCachedFileIcon/*=false*/) {
  // Won't work if we don't copy the name
  // for (char *c = &name[0]; *c; c++) *c = tolower(*c);

  char file_name[strlen(name) + 1]; // Room for filename and null
  sprintf_P(file_name, "%s", name);
  char file_path[strlen(name) + 1 + MAXPATHNAMELENGTH]; // Room for path, filename and null
  sprintf_P(file_path, "%s/%s", card.getWorkDirName(), file_name);

  // Check if cached
  bool use_cache = preview_type == Thumnail_Icon;
  if (use_cache) {
    auto it = image_cache.find(file_path+to_string(Thumnail_Icon));
    if (it != image_cache.end()) { // already cached
      if (it->second == 0) return false; // no image available
      *address = it->second;
      return true;
    } else if (onlyCachedFileIcon) return false;
  }

  const uint16_t buff_size = 256;
  char public_buf[buff_size+1];
  uint8_t output_buffer[6144];
  uint32_t position_in_file = 0;
  char *encoded_image = NULL;

  card.openFileRead(file_name);
  uint8_t n_reads = 0;
  int16_t data_read = card.read(public_buf, buff_size);
  card.setIndex(card.getIndex()+data_read);
  char key[31] = "";
  switch (preview_type) {
    case Thumnail_Icon: strcpy_P(key, "; jpeg thumbnail begin 50x50"); break;
    case Thumnail_Preview: strcpy_P(key, "; jpeg thumbnail begin 180x180"); break;
  }
  while(n_reads < 16 && data_read) { // Max 16 passes so we don't loop forever
  if (Encoder_ReceiveAnalyze() != ENCODER_DIFF_NO) return false;
    encoded_image = strstr(public_buf, key);
    if (encoded_image) {
      uint32_t index_bw = &public_buf[buff_size] - encoded_image;
      position_in_file = card.getIndex() - index_bw;
      break;
    }

    card.setIndex(card.getIndex()-32);
    data_read = card.read(public_buf, buff_size);
    card.setIndex(card.getIndex()+data_read);

    n_reads++;
  }

  // If we found the image, decode it
  if (encoded_image) {
  memset(public_buf, 0, sizeof(public_buf));
  card.setIndex(position_in_file+23); // ; thumbnail begin <move here>180x180 99999
  while (card.get() != ' '); // ; thumbnail begin 180x180 <move here>180x180

  char size_buf[10];
  for (size_t i = 0; i < sizeof(size_buf); i++)
  {
    uint8_t c = card.get();
    if (ISEOL(c)) {
      size_buf[i] = 0;
      break;
    }
    else
      size_buf[i] = c;
  }
  uint16_t image_size = atoi(size_buf);
  uint16_t stored_in_buffer = 0;
  uint8_t encoded_image_data[image_size+1];
  while (stored_in_buffer < image_size) {
    char c = card.get();
    if (ISEOL(c) || c == ';' || c == ' ') {
      continue;
    }
    else {
      encoded_image_data[stored_in_buffer] = c;
      stored_in_buffer++;
    }
  }

  encoded_image_data[stored_in_buffer] = 0;
  unsigned int output_size = decode_base64(encoded_image_data, output_buffer);
  if (next_available_address + output_size >= 0x7530) { // cache is full, invalidate it
    next_available_address = 0;
    image_cache.clear();
    SERIAL_ECHOLNPGM("Preview cache full, cleaning up...");
  }
  DWIN_Save_JPEG_in_SRAM((uint8_t *)output_buffer, output_size, next_available_address);
  *address = next_available_address;
  if(use_cache) {
    image_cache[file_path+to_string(preview_type)] = next_available_address;
    next_available_address += output_size + 1;
  }
  } else if (use_cache)  // If we didn't find the image, but we are using the cache, mark it as image not available
  {
    //image_cache[file_path+to_string(preview_type)] = 0;
    image_cache[file_path+to_string(preview_type)] = 0;
  }

  card.closefile();
  gcode.process_subcommands_now(F("M117")); // Clear the message sent by the card API
  return encoded_image;
}
#endif // DWIN_CREALITY_LCD_JYERSUI_GCODE_PREVIEW


void CrealityDWINClass::File_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  static uint8_t filescrl = 0;
  if (encoder_diffState == ENCODER_DIFF_NO) {
    if (selection > 0) {
      card.getfilename_sorted(SD_ORDER(selection - 1, card.get_num_Files()));
      char * const filename = card.longest_filename();
      size_t len = strlen(filename);
      int8_t pos = len;
      if (!card.flag.filenameIsDir)
        while (pos && filename[pos] != '.') pos--;
      if (pos > MENU_CHAR_LIMIT) {
        static millis_t time = 0;
        if (PENDING(millis(), time)) return;
        time = millis() + 200;
        pos -= filescrl;
        len = _MIN((size_t)pos, (size_t)MENU_CHAR_LIMIT);
        char name[len + 1];
        if (pos >= 0) {
          LOOP_L_N(i, len) name[i] = filename[i + filescrl];
        }
        else {
          LOOP_L_N(i, MENU_CHAR_LIMIT + pos) name[i] = ' ';
          LOOP_S_L_N(i, MENU_CHAR_LIMIT + pos, MENU_CHAR_LIMIT) name[i] = filename[i - (MENU_CHAR_LIMIT + pos)];
        }
        name[len] = '\0';
        DWIN_Draw_Rectangle(1, Color_Bg_Black, LBLX, MBASE(selection - scrollpos) - 14, 271, MBASE(selection - scrollpos) + 28);
        //Draw_Menu_Item(selection - scrollpos, card.flag.filenameIsDir ? ICON_More : ICON_File, name);
        Draw_Menu_Item(selection-scrollpos, card.flag.filenameIsDir ? ICON_More : ICON_File, name, NULL, NULL, false, true);
        if (-pos >= MENU_CHAR_LIMIT) filescrl = 0;
        filescrl++;
        DWIN_UpdateLCD();
      }
    }
    return;
  }
  if (encoder_diffState == ENCODER_DIFF_CW && selection < card.get_num_Files()) {
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
    if (selection > 0) {
      DWIN_Draw_Rectangle(1, Color_Bg_Black, LBLX, MBASE(selection - scrollpos) - 14, 271, MBASE(selection - scrollpos) + 28);
      Draw_SD_Item(selection, selection - scrollpos);
    }
    filescrl = 0;
    selection++; // Select Down
    if (selection > scrollpos + MROWS) {
      scrollpos++;
      DWIN_Frame_AreaMove(1, 2, MLINE, Color_Bg_Black, 0, 31, DWIN_WIDTH, 349);
      Draw_SD_Item(selection, selection - scrollpos);
    }

    #if ENABLED(DWIN_CREALITY_LCD_JYERSUI_GCODE_PREVIEW)
      thumbtime = millis() + SCROLL_WAIT;
      name_scroll_time = millis() + SCROLL_WAIT;
    #endif

    DWIN_Draw_Rectangle(1, GetColor(eeprom_settings.cursor_color, Rectangle_Color), 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
  }
  else if (encoder_diffState == ENCODER_DIFF_CCW && selection > 0) {
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
    DWIN_Draw_Rectangle(1, Color_Bg_Black, LBLX, MBASE(selection - scrollpos) - 14, 271, MBASE(selection - scrollpos) + 28);
    Draw_SD_Item(selection, selection - scrollpos);
    filescrl = 0;
    selection--; // Select Up
    if (selection < scrollpos) {
      scrollpos--;
      DWIN_Frame_AreaMove(1, 3, MLINE, Color_Bg_Black, 0, 31, DWIN_WIDTH, 349);
      Draw_SD_Item(selection, selection - scrollpos);
    }

    #if ENABLED(DWIN_CREALITY_LCD_JYERSUI_GCODE_PREVIEW)
      thumbtime = millis() + SCROLL_WAIT;
      name_scroll_time = millis() + SCROLL_WAIT;
    #endif

    DWIN_Draw_Rectangle(1, GetColor(eeprom_settings.cursor_color, Rectangle_Color), 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
  }
  else if (encoder_diffState == ENCODER_DIFF_ENTER) {
    if (selection == 0) {
      if (card.flag.workDirIsRoot) {
        process = Main;
        Draw_Main_Menu();
      }
      else {
        card.cdup();
        Draw_SD_List();
      }
    }
    else {
      card.getfilename_sorted(SD_ORDER(selection - 1, card.get_num_Files()));
      if (card.flag.filenameIsDir) {
        card.cd(card.filename);
        Draw_SD_List();
      }
      else {
        #if ENABLED(DWIN_CREALITY_LCD_JYERSUI_GCODE_PREVIEW)
        uint16_t image_address;
        bool has_preview = find_and_decode_gcode_preview(card.filename, Thumnail_Preview, &image_address);
        Popup_Handler(ConfirmStartPrint, has_preview);
        Draw_Title("Print file ?");
        if (has_preview) {
          DWIN_SRAM_Memory_Icon_Display(48,78,image_address);
        }
        else {
          gcode.process_subcommands_now(F("M117 Preview not found."));
        }
       #else
        card.openAndPrintFile(card.filename);
       #endif
      }
    }
  }
  DWIN_UpdateLCD();
}

void CrealityDWINClass::Print_Screen_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW && selection < PRINT_COUNT - 1) {
    selection++; // Select Down
    Print_Screen_Icons();
  }
  else if (encoder_diffState == ENCODER_DIFF_CCW && selection > 0) {
    selection--; // Select Up
    Print_Screen_Icons();
  }
  else if (encoder_diffState == ENCODER_DIFF_ENTER) {
    switch (selection) {
      case PRINT_SETUP:
        Draw_Menu(Tune);
        Update_Status_Bar(true);
        break;
      case PRINT_PAUSE_RESUME:
        if (paused) {
          if (sdprint) {
            wait_for_user = false;
            #if ENABLED(PARK_HEAD_ON_PAUSE)
              card.startOrResumeFilePrinting();
              TERN_(POWER_LOSS_RECOVERY, recovery.prepare());
            #else
              #if HAS_HEATED_BED
                cmd[sprintf_P(cmd, PSTR("M140 S%i"), pausebed)] = '\0';
                gcode.process_subcommands_now(cmd);
              #endif
              #if HAS_EXTRUDERS
                cmd[sprintf_P(cmd, PSTR("M109 S%i"), pausetemp)] = '\0';
                gcode.process_subcommands_now(cmd);
              #endif
              TERN_(HAS_FAN, thermalManager.fan_speed[0] = pausefan);
              planner.synchronize();
              TERN_(SDSUPPORT, queue.inject(F("M24")));
            #endif
          }
          else {
            TERN_(HOST_ACTION_COMMANDS, hostui.resume());
          }
          Draw_Print_Screen();
        }
        else
          Popup_Handler(Pause);
        break;
      case PRINT_STOP: Popup_Handler(Stop); break;
    }
  }
  DWIN_UpdateLCD();
}

void CrealityDWINClass::Popup_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW && selection < 1) {
    selection++;
    Popup_Select();
  }
  else if (encoder_diffState == ENCODER_DIFF_CCW && selection > 0) {
    selection--;
    Popup_Select();
  }
  else if (encoder_diffState == ENCODER_DIFF_ENTER) {
    switch (popup) {
      case Pause:
        if (selection == 0) {
          if (sdprint) {
            #if ENABLED(POWER_LOSS_RECOVERY)
              if (recovery.enabled) recovery.save(true);
            #endif
            #if ENABLED(PARK_HEAD_ON_PAUSE)
              Popup_Handler(Home, true);
              #if ENABLED(SDSUPPORT)
                if (IS_SD_PRINTING()) card.pauseSDPrint();
              #endif
              planner.synchronize();
              queue.inject(F("M125 P1"));
              planner.synchronize();
            #else
              queue.inject(F("M25"));
              TERN_(HAS_HOTEND, pausetemp = thermalManager.temp_hotend[0].target);
              TERN_(HAS_HEATED_BED, pausebed = thermalManager.temp_bed.target);
              TERN_(HAS_FAN, pausefan = thermalManager.fan_speed[0]);
              thermalManager.cooldown();
            #endif
          }
          else {
            TERN_(HOST_ACTION_COMMANDS, hostui.pause());
          }
        }
        Draw_Print_Screen();
        break;
      case Stop:
        if (selection == 0) {
          if (sdprint) {
            ui.abort_print();
            thermalManager.cooldown();
          }
          else {
            TERN_(HOST_ACTION_COMMANDS, hostui.cancel());
          }
        }
        else
          Draw_Print_Screen();
        break;
      case Resume:
        if (selection == 0)
            queue.inject( TERN(POWER_LOSS_RECOVERY, F("M1000"), F("M25")) );
        else {
            queue.inject( TERN(POWER_LOSS_RECOVERY, F("M1000 C"), F("M25")) );
          Draw_Main_Menu();
        }
        break;

      #if HAS_HOTEND
        case ETemp:
          if (selection == 0) {
//            thermalManager.setTargetHotend(EXTRUDE_MINTEMP, 0);
//            thermalManager.set_fan_speed(0, MAX_FAN_SPEED);
            Draw_Menu(PreheatHotend);
          }
          else
            Redraw_Menu(true, true, false);
          break;
      #endif

      #if HAS_BED_PROBE
        case ManualProbing:
          if (selection == 0) {
            char buf[80];
            const float dif = probe.probe_at_point(current_position.x, current_position.y, PROBE_PT_STOW, 0, false) - corner_avg;
            sprintf_P(buf, dif > 0 ? PSTR("Corner is %smm high") : PSTR("Corner is %smm low"), dtostrf(abs(dif), 1, 3, str_1));
            Update_Status(buf);
          }
          else {
            Redraw_Menu(true, true, false);
            Update_Status("");
          }
          break;
      #endif

      #if ENABLED(ADVANCED_PAUSE_FEATURE)
        case ConfFilChange:
          if (selection == 0) {
            if (thermalManager.temp_hotend[0].target < thermalManager.extrude_min_temp)
              Popup_Handler(ETemp);
            else {
              if (thermalManager.temp_hotend[0].celsius < thermalManager.temp_hotend[0].target - 2) {
                Popup_Handler(Heating);
                thermalManager.wait_for_hotend(0);
              }
              Popup_Handler(FilChange);
              sprintf_P(cmd, PSTR("M600 B1 R%i"), thermalManager.temp_hotend[0].target);
              gcode.process_subcommands_now(cmd);
            }
          }
          else
            Redraw_Menu(true, true, false);
          break;
        case PurgeMore:
          if (selection == 0) {
            pause_menu_response = PAUSE_RESPONSE_EXTRUDE_MORE;
            Popup_Handler(FilChange);
          }
          else {
            pause_menu_response = PAUSE_RESPONSE_RESUME_PRINT;
            if (printing) Popup_Handler(Resuming);
            else Redraw_Menu(true, true, (active_menu==PreheatHotend));
          }
          break;
        case ConfirmStartPrint:
          if (selection==0) {
            card.openAndPrintFile(card.filename);}
          else{
            Redraw_Menu(true, true, true);
            gcode.process_subcommands_now(F("M117"));}
          break;

      #endif // ADVANCED_PAUSE_FEATURE

      #if HAS_MESH
        case SaveLevel:
          if (selection == 0) {
            #if ENABLED(AUTO_BED_LEVELING_UBL)
              gcode.process_subcommands_now(F("G29 S"));
              planner.synchronize();
              AudioFeedback(true);
            #else
              AudioFeedback(settings.save());
            #endif
          }
          Draw_Menu(Leveling, LEVELING_GET_MESH);
          break;
      #endif

      #if ENABLED(AUTO_BED_LEVELING_UBL)
        case MeshSlot:
          if (selection == 0) ubl.storage_slot = 0;
          Redraw_Menu(true, true);
          break;
      #endif
      default: break;
    }
  }
  DWIN_UpdateLCD();
}

void CrealityDWINClass::Confirm_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_ENTER) {
    switch (popup) {
      case Complete:
        Draw_Main_Menu();
        break;
      case FilInsert:
        Popup_Handler(FilChange);
        wait_for_user = false;
        break;
      case HeaterTime:
        Popup_Handler(Heating);
        wait_for_user = false;
        break;
      default:
        Redraw_Menu(true, true, false);
        wait_for_user = false;
        break;
    }
  }
  DWIN_UpdateLCD();
}

void CrealityDWINClass::Keyboard_Control() {
  const uint8_t keyboard_size = 34;
  static uint8_t key_selection = 0, cursor = 0;
  static char string[31];
  static bool uppercase = false, locked = false;
  if (reset_keyboard) {
    if (strcmp(stringpointer, "-") == 0) stringpointer[0] = '\0';
    key_selection = 0, cursor = strlen(stringpointer);
    uppercase = false, locked = false;
    reset_keyboard = false;
    strcpy(string, stringpointer);
  }
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW) {
    Draw_Keys(key_selection, false, uppercase, locked);
    key_selection++;
    if (key_selection > keyboard_size) key_selection = 0;
    Draw_Keys(key_selection, true, uppercase, locked);
  }
  else if (encoder_diffState == ENCODER_DIFF_CCW) {
    Draw_Keys(key_selection, false, uppercase, locked);
    if (key_selection == 0) key_selection = keyboard_size+1;
    key_selection--;
    Draw_Keys(key_selection, true, uppercase, locked);
  }
  else if (encoder_diffState == ENCODER_DIFF_ENTER) {
    if (key_selection < 28) {
      if (key_selection == 19) {
        if (!numeric_keyboard) {
          if (locked) {
            uppercase = false, locked = false;
            Draw_Keyboard(keyboard_restrict, false, key_selection, uppercase, locked);
          } else if (uppercase) {
            locked = true;
            Draw_Keyboard(keyboard_restrict, false, key_selection, uppercase, locked);
          }
          else {
            uppercase = true;
            Draw_Keyboard(keyboard_restrict, false, key_selection, uppercase, locked);
          }
        }
      }
      else if (key_selection == 27) {
        cursor--;
        string[cursor] = '\0';
      }
      else {
        uint8_t index = key_selection;
        if (index > 19) index--;
        if (index > 27) index--;
        const char *keys;
        if (numeric_keyboard) keys = "1234567890&<>(){}[]*\"\':;!?";
        else keys = (uppercase) ? "QWERTYUIOPASDFGHJKLZXCVBNM" : "qwertyuiopasdfghjklzxcvbnm";
        if (!(keyboard_restrict && numeric_keyboard && index > 9)) {
          string[cursor] = keys[index];
          cursor++;
          string[cursor] = '\0';
        }
        if (!locked && uppercase) {
          uppercase = false;
          Draw_Keyboard(keyboard_restrict, false, key_selection, uppercase, locked);
        }
      }
    }
    else {
      switch (key_selection) {
        case 28:
          if (!numeric_keyboard) uppercase = false, locked = false;
          Draw_Keyboard(keyboard_restrict, !numeric_keyboard, key_selection, uppercase, locked);
          break;
        case 29:
          string[cursor] = '-';
          cursor++;
          string[cursor] = '\0';
          break;
        case 30:
          string[cursor] = '_';
          cursor++;
          string[cursor] = '\0';
          break;
        case 31:
          if (!keyboard_restrict) {
            string[cursor] = ' ';
            cursor++;
            string[cursor] = '\0';
          }
          break;
        case 32:
          if (!keyboard_restrict) {
            string[cursor] = '.';
            cursor++;
            string[cursor] = '\0';
          }
          break;
        case 33:
          if (!keyboard_restrict) {
            string[cursor] = '/';
            cursor++;
            string[cursor] = '\0';
          }
          break;
        case 34:
          if (string[0] == '\0') strcpy(string, "-");
          strcpy(stringpointer, string);
          process = Menu;
          if (KEY_Y_START < STATUS_Y) DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, KEY_Y_START, DWIN_WIDTH, STATUS_Y); // Keyboard is higher than StatusArea
          Draw_Status_Area(true);
          Update_Status_Bar(true);
          break;
      }
    }
    if (strlen(string) > maxstringlen) string[maxstringlen] = '\0', cursor = maxstringlen;
    Draw_String(string, selection, (process==Keyboard), (maxstringlen > 10));
  }
  DWIN_UpdateLCD();
}


/* In-Menu Value Modification */

void CrealityDWINClass::Setup_Value(float value, float min, float max, float unit, uint8_t type) {
  if (TERN0(HAS_HOTEND, valuepointer == &thermalManager.temp_hotend[0].pid.Ki) || TERN0(HAS_HEATED_BED, valuepointer == &thermalManager.temp_bed.pid.Ki))
    tempvalue = unscalePID_i(value) * unit;
  else if (TERN0(HAS_HOTEND, valuepointer == &thermalManager.temp_hotend[0].pid.Kd) || TERN0(HAS_HEATED_BED, valuepointer == &thermalManager.temp_bed.pid.Kd))
    tempvalue = unscalePID_d(value) * unit;
  else
    tempvalue = value * unit;
  valuemin = min;
  valuemax = max;
  valueunit = unit;
  valuetype = type;
  process = Value;
  EncoderRate.enabled = true;
  Draw_Float(tempvalue / unit, selection - scrollpos, true, valueunit);
}

void CrealityDWINClass::Modify_Value(float &value, float min, float max, float unit, void (*f)()/*=nullptr*/) {
  valuepointer = &value;
  funcpointer = f;
  Setup_Value((float)value, min, max, unit, 0);
}
void CrealityDWINClass::Modify_Value(uint8_t &value, float min, float max, float unit, void (*f)()/*=nullptr*/) {
  valuepointer = &value;
  funcpointer = f;
  Setup_Value((float)value, min, max, unit, 1);
}
void CrealityDWINClass::Modify_Value(uint16_t &value, float min, float max, float unit, void (*f)()/*=nullptr*/) {
  valuepointer = &value;
  funcpointer = f;
  Setup_Value((float)value, min, max, unit, 2);
}
void CrealityDWINClass::Modify_Value(int16_t &value, float min, float max, float unit, void (*f)()/*=nullptr*/) {
  valuepointer = &value;
  funcpointer = f;
  Setup_Value((float)value, min, max, unit, 3);
}
void CrealityDWINClass::Modify_Value(uint32_t &value, float min, float max, float unit, void (*f)()/*=nullptr*/) {
  valuepointer = &value;
  funcpointer = f;
  Setup_Value((float)value, min, max, unit, 4);
}
void CrealityDWINClass::Modify_Value(int8_t &value, float min, float max, float unit, void (*f)()/*=nullptr*/) {
  valuepointer = &value;
  funcpointer = f;
  Setup_Value((float)value, min, max, unit, 5);
}

void CrealityDWINClass::Modify_Option(uint8_t value, const char * const * options, uint8_t max) {
  tempvalue = value;
  valuepointer = const_cast<const char * *>(options);
  valuemin = 0;
  valuemax = max;
  process = Option;
  EncoderRate.enabled = true;
  Draw_Option(value, options, selection - scrollpos, true);
}

void CrealityDWINClass::Modify_String(char * string, uint8_t maxlength, bool restrict) {
  stringpointer = string;
  maxstringlen = maxlength;
  reset_keyboard = true;
  Draw_Keyboard(restrict, false);
  Draw_String(string, selection, true, (maxstringlen > 10));
}

/* Main Functions */

void CrealityDWINClass::Update_Status(const char * const text) {

  DEBUG_ECHOLNPGM("CrealityDWINClass::Update_Status (", text, ")");

  char header[4];
  LOOP_L_N(i, 3) header[i] = text[i];
  header[3] = '\0';
  //if (strcmp_P(header, PSTR("<F>")) == 0) {
  if (false) {
    LOOP_L_N(i, _MIN((size_t)LONG_FILENAME_LENGTH, strlen(text))) filename[i] = text[i + 3];
    filename[_MIN((size_t)LONG_FILENAME_LENGTH - 1, strlen(text))] = '\0';
    Draw_Print_Filename(true);
  }
  else {
    LOOP_L_N(i, _MIN((size_t)64, strlen(text))) statusmsg[i] = text[i];
    statusmsg[_MIN((size_t)64, strlen(text))] = '\0';
  }
}

void CrealityDWINClass::Start_Print(bool sd) {
  sdprint = sd;
  if (!printing) {
    printing = true;
    statusmsg[0] = '\0';
    if (sd) {
      #if ENABLED(POWER_LOSS_RECOVERY)
        if (recovery.valid()) {
          SdFile *diveDir = nullptr;
          const char * const fname = card.diveToFile(true, diveDir, recovery.info.sd_filename);
          card.selectFileByName(fname);
        }
      #endif
      strcpy_P(filename, card.longest_filename());
    }
    else
      strcpy_P(filename, "Host Print");
    TERN_(LCD_SET_PROGRESS_MANUALLY, ui.set_progress(0));
    TERN_(USE_M73_REMAINING_TIME, ui.set_remaining_time(0));
    Draw_Print_Screen();
  }
}

void CrealityDWINClass::Stop_Print() {
  printing = false;
  sdprint = false;
  thermalManager.cooldown();
  TERN_(LCD_SET_PROGRESS_MANUALLY, ui.set_progress(100 * (PROGRESS_SCALE)));
  TERN_(USE_M73_REMAINING_TIME, ui.set_remaining_time(0));
  Draw_Print_confirm();
}

void CrealityDWINClass::Update() {
  State_Update();
  Screen_Update();
  switch (process) {
    case Main:      Main_Menu_Control();    break;
    case Menu:      Menu_Control();         break;
    case Value:     Value_Control();        break;
    case Option:    Option_Control();       break;
    case File:      File_Control();         break;
    case Print:     Print_Screen_Control(); break;
    case Popup:     Popup_Control();        break;
    case Confirm:   Confirm_Control();      break;
    case Keyboard:  Keyboard_Control();     break;
  }
}

void MarlinUI::update() { CrealityDWIN.Update(); }

#if HAS_LCD_BRIGHTNESS
  void MarlinUI::_set_brightness() { DWIN_LCD_Brightness(backlight ? brightness : 0); }
#endif

void CrealityDWINClass::State_Update() {
  if (DEBUG_INFOLINE(2)) {
    DEBUG_ECHOLNPGM("CrealityDWINClass::State_Update (", dbg_InfoLine1, ")" );
  }
  if ((print_job_timer.isRunning() || print_job_timer.isPaused()) != printing) {
    if (!printing) Start_Print(card.isFileOpen() || TERN0(POWER_LOSS_RECOVERY, recovery.valid()));
    else Stop_Print();
  }
  if (print_job_timer.isPaused() != paused) {
    paused = print_job_timer.isPaused();
    if (process == Print) Print_Screen_Icons();
    if (process == Wait && !paused) Redraw_Menu(true, true);
  }
  if (wait_for_user && !(process == Confirm) && !print_job_timer.isPaused())
    Confirm_Handler(UserInput);
  #if ENABLED(ADVANCED_PAUSE_FEATURE)
    if (process == Popup && popup == PurgeMore) {
      if (pause_menu_response == PAUSE_RESPONSE_EXTRUDE_MORE)
        Popup_Handler(FilChange);
      else if (pause_menu_response == PAUSE_RESPONSE_RESUME_PRINT) {
        if (printing) Popup_Handler(Resuming);
        else Redraw_Menu(true, true, (active_menu==PreheatHotend));
      }
    }
  #endif
  #if ENABLED(FILAMENT_RUNOUT_SENSOR)
    static bool ranout = false;
    if (runout.filament_ran_out != ranout) {
      ranout = runout.filament_ran_out;
      if (ranout) Popup_Handler(Runout);
    }
  #endif
}

void CrealityDWINClass::Screen_Update() {
  const millis_t ms = millis();
  static millis_t scrltime = 0;
  if (ELAPSED(ms, scrltime)) {
    scrltime = ms + 200;
    if (process != Keyboard) Update_Status_Bar();
    if (process == Print) Draw_Print_Filename();
  }

  static millis_t statustime = 0;
  if (ELAPSED(ms, statustime) && process != Keyboard) {
    statustime = ms + 500;
    Draw_Status_Area();
  }

  static millis_t printtime = 0;
  if (ELAPSED(ms, printtime)) {
    printtime = ms + 1000;
    if (process == Print) {
      Draw_Print_ProgressBar();
      Draw_Print_ProgressElapsed();
      TERN_(USE_M73_REMAINING_TIME, Draw_Print_ProgressRemain());
    }
  }

  static bool mounted = card.isMounted();
  if (mounted != card.isMounted()) {
    mounted = card.isMounted();
    #if ENABLED(DWIN_CREALITY_LCD_JYERSUI_GCODE_PREVIEW)
    image_cache.clear();
    #endif
    if (process == File)
      Draw_SD_List();
  }

  #if ENABLED(DWIN_CREALITY_LCD_JYERSUI_GCODE_PREVIEW)
  if (eeprom_settings.show_gcode_thumbnails && ELAPSED(millis(), thumbtime)) {
    thumbtime = millis() + 60000;
    if (process == File){
      // Draw_SD_List(!mounted, selection, scrollpos);
      if (selection-scrollpos > MROWS) scrollpos = selection - MROWS;
      LOOP_L_N(i, _MIN(card.get_num_Files()+1, TROWS)) {
        if (Encoder_ReceiveAnalyze() != ENCODER_DIFF_NO) break;
        if (i+scrollpos == 0) {
          if (card.flag.workDirIsRoot)
            Draw_Menu_Item(0, ICON_Back, "Back");
          else
            Draw_Menu_Item(0, ICON_Back, "..");
        } else {
          card.getfilename_sorted(SD_ORDER(i+scrollpos-1, card.get_num_Files()));

          Draw_Menu_Item(i, card.flag.filenameIsDir ? ICON_More : ICON_File);
        }
        DWIN_UpdateLCD();
      }
      DWIN_Draw_Rectangle(1, GetColor(eeprom_settings.cursor_color, Rectangle_Color), 0, MBASE(selection-scrollpos) - 18, 8, MBASE(selection-scrollpos) + 31);
    }
  }
  #endif

  #if HAS_HOTEND
    static int16_t hotendtarget = -1;
  #endif
  #if HAS_HEATED_BED
    static int16_t bedtarget = -1;
  #endif
  #if HAS_FAN
    static int16_t fanspeed = -1;
  #endif

  #if HAS_ZOFFSET_ITEM
    static float lastzoffset = zoffsetvalue;
    if (zoffsetvalue != lastzoffset) {
      lastzoffset = zoffsetvalue;
      #if HAS_BED_PROBE
        probe.offset.z = zoffsetvalue;
      #else
        set_home_offset(Z_AXIS, -zoffsetvalue);
      #endif
    }

    #if HAS_BED_PROBE
      if (probe.offset.z != lastzoffset)
        zoffsetvalue = lastzoffset = probe.offset.z;
    #else
      if (-home_offset.z != lastzoffset)
        zoffsetvalue = lastzoffset = -home_offset.z;
    #endif
  #endif // HAS_ZOFFSET_ITEM

  if (process == Menu || process == Value) {
    switch (active_menu) {
      case TempMenu:
        #if HAS_HOTEND
          if (thermalManager.temp_hotend[0].target != hotendtarget) {
            hotendtarget = thermalManager.temp_hotend[0].target;
            if (scrollpos <= TEMP_HOTEND && TEMP_HOTEND <= scrollpos + MROWS) {
              if (process != Value || selection != TEMP_HOTEND - scrollpos)
                Draw_Float(thermalManager.temp_hotend[0].target, TEMP_HOTEND - scrollpos, false, 1);
            }
          }
        #endif
        #if HAS_HEATED_BED
          if (thermalManager.temp_bed.target != bedtarget) {
            bedtarget = thermalManager.temp_bed.target;
            if (scrollpos <= TEMP_BED && TEMP_BED <= scrollpos + MROWS) {
              if (process != Value || selection != TEMP_HOTEND - scrollpos)
                Draw_Float(thermalManager.temp_bed.target, TEMP_BED - scrollpos, false, 1);
            }
          }
        #endif
        #if HAS_FAN
          if (thermalManager.fan_speed[0] != fanspeed) {
            fanspeed = thermalManager.fan_speed[0];
            if (scrollpos <= TEMP_FAN && TEMP_FAN <= scrollpos + MROWS) {
              if (process != Value || selection != TEMP_HOTEND - scrollpos)
                Draw_Float(thermalManager.fan_speed[0], TEMP_FAN - scrollpos, false, 1);
            }
          }
        #endif
        break;
      case Tune:
        #if HAS_HOTEND
          if (thermalManager.temp_hotend[0].target != hotendtarget) {
            hotendtarget = thermalManager.temp_hotend[0].target;
            if (scrollpos <= TUNE_HOTEND && TUNE_HOTEND <= scrollpos + MROWS) {
              if (process != Value || selection != TEMP_HOTEND - scrollpos)
                Draw_Float(thermalManager.temp_hotend[0].target, TUNE_HOTEND - scrollpos, false, 1);
            }
          }
        #endif
        #if HAS_HEATED_BED
          if (thermalManager.temp_bed.target != bedtarget) {
            bedtarget = thermalManager.temp_bed.target;
            if (scrollpos <= TUNE_BED && TUNE_BED <= scrollpos + MROWS) {
              if (process != Value || selection != TEMP_HOTEND - scrollpos)
                Draw_Float(thermalManager.temp_bed.target, TUNE_BED - scrollpos, false, 1);
            }
          }
        #endif
        #if HAS_FAN
          if (thermalManager.fan_speed[0] != fanspeed) {
            fanspeed = thermalManager.fan_speed[0];
            if (scrollpos <= TUNE_FAN && TUNE_FAN <= scrollpos + MROWS) {
              if (process != Value || selection != TEMP_HOTEND - scrollpos)
                Draw_Float(thermalManager.fan_speed[0], TUNE_FAN - scrollpos, false, 1);
            }
          }
        #endif
        break;
    }
  }
}

void CrealityDWINClass::AudioFeedback(const bool success/*=true*/) {
  if (success) {
    if (ui.buzzer_enabled) {
      BUZZ(100, 659);
      BUZZ( 10,   0);
      BUZZ(100, 698);
    }
    else Update_Status("Success");
  }
  else if (ui.buzzer_enabled)
    BUZZ(40, 440);
  else
    Update_Status("Failed");
}

void CrealityDWINClass::Save_Settings(char *buff) {
  TERN_(AUTO_BED_LEVELING_UBL, eeprom_settings.tilt_grid_size = mesh_conf.tilt_grid - 1);
  eeprom_settings.corner_pos = corner_pos * 10;
  #if ENABLED(HOST_ACTION_COMMANDS)
    eeprom_settings.host_action_label_1 = Encode_String(action1);
    eeprom_settings.host_action_label_2 = Encode_String(action2);
    eeprom_settings.host_action_label_3 = Encode_String(action3);
  #endif

  TERN_(PREVENT_COLD_EXTRUSION, eeprom_settings.extrude_min_temp = _MIN(thermalManager.extrude_min_temp, 255));

  memcpy(buff, &eeprom_settings, _MIN(sizeof(eeprom_settings), eeprom_data_size));
}

void CrealityDWINClass::Load_Settings(const char *buff) {
  memcpy(&eeprom_settings, buff, _MIN(sizeof(eeprom_settings), eeprom_data_size));
  TERN_(AUTO_BED_LEVELING_UBL, mesh_conf.tilt_grid = eeprom_settings.tilt_grid_size + 1);
  if (eeprom_settings.corner_pos == 0) eeprom_settings.corner_pos = 325;
  corner_pos = eeprom_settings.corner_pos / 10.0f;
  #if ENABLED(HOST_ACTION_COMMANDS)
    Decode_String(eeprom_settings.host_action_label_1, action1);
    Decode_String(eeprom_settings.host_action_label_2, action2);
    Decode_String(eeprom_settings.host_action_label_3, action3);
  #endif

  TERN_(PREVENT_COLD_EXTRUSION, thermalManager.extrude_min_temp = eeprom_settings.extrude_min_temp);

  Redraw_Screen();
  #if ENABLED(POWER_LOSS_RECOVERY)
    static bool init = true;
    if (init) {
      init = false;
      queue.inject(F("M1000 S"));
    }
  #endif
}

void CrealityDWINClass::Reset_Settings() {
  eeprom_settings.time_format_textual = false;
  TERN_(AUTO_BED_LEVELING_UBL, eeprom_settings.tilt_grid_size = 0);
  eeprom_settings.corner_pos = 300;
  eeprom_settings.cursor_color = 0;
  eeprom_settings.menu_split_line = 0;
  eeprom_settings.menu_top_bg = 0;
  eeprom_settings.menu_top_txt = 0;
  eeprom_settings.highlight_box = 0;
  eeprom_settings.progress_percent = 0;
  eeprom_settings.progress_time = 0;
  eeprom_settings.status_bar_text = 0;
  eeprom_settings.status_area_text = 0;
  eeprom_settings.coordinates_text = 0;
  eeprom_settings.coordinates_split_line = 0;
  #if ENABLED(HOST_ACTION_COMMANDS)
    eeprom_settings.host_action_label_1 = 0;
    eeprom_settings.host_action_label_2 = 0;
    eeprom_settings.host_action_label_3 = 0;
    action1[0] = action2[0] = action3[0] = '-';
  #endif
  TERN_(PREVENT_COLD_EXTRUSION, thermalManager.extrude_min_temp = eeprom_settings.extrude_min_temp = EXTRUDE_MINTEMP);
  TERN_(AUTO_BED_LEVELING_UBL, mesh_conf.tilt_grid = eeprom_settings.tilt_grid_size + 1);
  corner_pos = eeprom_settings.corner_pos / 10.0f;
  TERN_(SOUND_MENU_ITEM, ui.buzzer_enabled = true);
  #if ENABLED(DWIN_CREALITY_LCD_JYERSUI_GCODE_PREVIEW)
    eeprom_settings.show_gcode_thumbnails = true;
  #endif
  eeprom_settings.show_debug_on_LCD = false;
  Redraw_Screen();
}

void MarlinUI::init_lcd() {
  delay(800);
  SERIAL_ECHOPGM("\nDWIN handshake ");
  if (DWIN_Handshake()) SERIAL_ECHOLNPGM("ok."); else SERIAL_ECHOLNPGM("error.");
  DWIN_Frame_SetDir(1); // Orientation 90°
  DWIN_UpdateLCD();     // Show bootscreen (first image)
  Encoder_Configuration();
  for (uint16_t t = 0; t <= 100; t += 2) {
    DWIN_ICON_Show(ICON, ICON_Bar, 15, 260);
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 15 + t * 242 / 100, 260, 257, 280);
    DWIN_UpdateLCD();
    delay(20);
  }
  DWIN_JPG_CacheTo1(Language_English);
  CrealityDWIN.Redraw_Screen();
}

#if ENABLED(ADVANCED_PAUSE_FEATURE)
  void MarlinUI::pause_show_message(const PauseMessage message, const PauseMode mode/*=PAUSE_MODE_SAME*/, const uint8_t extruder/*=active_extruder*/) {

  DEBUG_ECHOLNPGM("MarlinUI::pause_show_message (message=", message, ", mode=", mode, ")");

    switch (message) {
      case PAUSE_MESSAGE_INSERT:  CrealityDWIN.Update_Status(GET_TEXT(MSG_FILAMENT_CHANGE_INSERT));
                                  CrealityDWIN.Confirm_Handler(FilInsert);
                                  break;

      case PAUSE_MESSAGE_PURGE:   CrealityDWIN.Update_Status(GET_TEXT(MSG_FILAMENT_CHANGE_PURGE));
                                  break;

      case PAUSE_MESSAGE_OPTION:  CrealityDWIN.Update_Status(GET_TEXT(MSG_FILAMENT_CHANGE_OPTION_HEADER));
                                  pause_menu_response = PAUSE_RESPONSE_WAIT_FOR;
                                  CrealityDWIN.Popup_Handler(PurgeMore);
                                  break;

      case PAUSE_MESSAGE_HEAT:    CrealityDWIN.Update_Status(GET_TEXT(MSG_FILAMENT_CHANGE_HEAT));
                                  CrealityDWIN.Confirm_Handler(HeaterTime);
                                  break;

      case PAUSE_MESSAGE_WAITING: CrealityDWIN.Update_Status(GET_TEXT(MSG_ADVANCED_PAUSE_WAITING));
                                  CrealityDWIN.Draw_Print_Screen();
                                  break;

      case PAUSE_MESSAGE_PARKING: CrealityDWIN.Update_Status(GET_TEXT(MSG_PAUSE_PRINT_PARKING));
                                  CrealityDWIN.Popup_Handler(Home);
                                  break;
      case PAUSE_MESSAGE_CHANGING: CrealityDWIN.Update_Status(GET_TEXT(MSG_FILAMENT_CHANGE_INIT));
                                  break;
      case PAUSE_MESSAGE_UNLOAD:  CrealityDWIN.Update_Status(GET_TEXT(MSG_FILAMENT_CHANGE_UNLOAD));
                                  break;
      case PAUSE_MESSAGE_LOAD:    CrealityDWIN.Update_Status(GET_TEXT(MSG_FILAMENT_CHANGE_LOAD));
                                  break;
      case PAUSE_MESSAGE_RESUME:  CrealityDWIN.Update_Status(GET_TEXT(MSG_FILAMENT_CHANGE_RESUME));
                                  CrealityDWIN.Popup_Handler(Resume);
                                  break;
      case PAUSE_MESSAGE_STATUS:  CrealityDWIN.Update_Status("Pause message: Status");
                                  break;
      case PAUSE_MESSAGE_HEATING: CrealityDWIN.Update_Status(GET_TEXT(MSG_FILAMENT_CHANGE_HEATING));
                                  CrealityDWIN.Popup_Handler(Heating);
                                  break;

      default: break;
    }
  }
#endif

#endif // DWIN_CREALITY_LCD_JYERSUI

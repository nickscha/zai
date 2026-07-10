/* win32_zai.c - v0.1 - public domain data structures - nickscha 2026

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
#include "zai_types.h"
#include "zai.h"
#include "zai_geometry.h"
#include "zai_font.h"
#include "zai_string_builder.h"
#include "zai_profiler.h"
#include "zai_opengl.h"
#include "zai_camera.h"
#include "zai_noise.h"
#include "zai_surface_nets.h"
#include "zai_marching_cubes.h"
#include "zai_sparse_grid.h"
#include "zai_sdf_scene.h"
#include "zai_tiles.h"
#include "win32_zai_opengl.h"
#include "win32_zai_api.h"
#include "win32_zai_xinput.h"

/* Unfortunaly "modern" compilers sometimes inject memset intrinsics in the generated code
 * even if the application does not call memset and even with -fno-builtin, ... set.
 * Therefore we have to provide our own memset function.
 */
#ifdef _MSC_VER
#pragma function(memset)
#endif
void *memset(void *dest, i32 c, u32 count)
{
  s8 *bytes = (s8 *)dest;
  while (count--)
  {
    *bytes++ = (s8)c;
  }
  return dest;
}

ZAI_API f64 zai_profiler_time_ms(void)
{
  static i64 freq;

  i64 time;

  if (!freq)
  {
    QueryPerformanceFrequency(&freq);
  }

  QueryPerformanceCounter(&time);

  return ((f64)time * 1000.0) / (f64)freq;
}

/* #############################################################################
 * # [SECTION] Force Discrete GPU
 * #############################################################################
 */
__declspec(dllexport) u32 NvOptimusEnablement = 0x00000001;         /* NVIDIA Force discrete GPU */
__declspec(dllexport) i32 AmdPowerXpressRequestHighPerformance = 1; /* AMD Force discrete GPU    */

/* #############################################################################
 * # [SECTION] WIN32 specifiy functions
 * #############################################################################
 */
ZAI_API u8 win32_print(s8 *str)
{
  static u32 written;
  static void *log_file;

  if (!log_file)
  {
    log_file = CreateFileA("zai.log", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  }

  {
    s8 *p = str;
    u32 len = 0;

    while (*p++)
    {
      len++;
    }

    WriteFile(log_file, str, len, &written, 0);
  }

  return 1;
}

ZAI_API u8 *win32_file_read(s8 *filename, u32 *file_size_out)
{
  void *hFile = INVALID_HANDLE;
  u32 fileSize = 0;
  u32 bytesRead = 0;

  u8 *buffer = 0;
  i32 attempt;

  /* Retry loop for hot-reload: file might be locked or partially written */
  for (attempt = 0; attempt < 4; ++attempt)
  {
    hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);

    if (hFile != INVALID_HANDLE)
    {
      break;
    }

    Sleep(5); /* Small delay, adjust as needed */
  }

  if (hFile == INVALID_HANDLE)
  {
    return ZAI_NULL;
  }

  fileSize = GetFileSize(hFile, 0);

  if (fileSize == INVALID_FILE_SIZE || fileSize == 0)
  {
    CloseHandle(hFile);
    return ZAI_NULL;
  }

  buffer = (u8 *)VirtualAlloc(0, fileSize + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

  if (!buffer)
  {
    CloseHandle(hFile);
    return ZAI_NULL;
  }

  if (!ReadFile(hFile, buffer, fileSize, &bytesRead, 0) || bytesRead != fileSize)
  {
    VirtualFree(buffer, 0, MEM_RELEASE);
    CloseHandle(hFile);
    return ZAI_NULL;
  }

  buffer[fileSize] = '\0';

  *file_size_out = fileSize;
  CloseHandle(hFile);
  return buffer;
}

ZAI_API ZAI_INLINE FILETIME win32_file_mod_time(s8 *file)
{
  static FILETIME empty = {0, 0};
  WIN32_FILE_ATTRIBUTE_DATA fad;
  return GetFileAttributesExA(file, 0, &fad) ? fad.ftLastWriteTime : empty;
}

ZAI_API ZAI_INLINE u8 win32_enable_high_priority(void)
{
  /* TODO(nickscha): Check integrated plus discrete GPU
   *
   * It was noticed that on a device with a integrated GPU (intel)
   * and a discrete GPU (NVIDIA) the fans were spinning high even
   * though there was only 1.5% total CPU and 2.4% GPU usage.
   * Check carefully when these priority functions can be set safely.
   *
  if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
  {
    return 1;
  }

  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST))
  {
    return 1;
  }

  if (!SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED))
  {
    return 1;
  }
  */
  (void)HIGH_PRIORITY_CLASS;
  (void)THREAD_PRIORITY_HIGHEST;
  (void)ES_CONTINUOUS;
  (void)ES_DISPLAY_REQUIRED;
  (void)ES_SYSTEM_REQUIRED;

  return 1;
}

ZAI_API ZAI_INLINE u8 win32_enable_dpi_awareness(void)
{
  /* Try Windows 10 / 11 (Per-Monitor V2) */
  void *user32 = GetModuleHandleA("user32.dll");

  void *shcore;

  if (user32)
  {
    typedef i32(__stdcall * SetProcessDpiAwarenessContextProc)(void *);
    SetProcessDpiAwarenessContextProc setProcessDpiAwarenessContext;

    *(void **)(&setProcessDpiAwarenessContext) = GetProcAddress(user32, "SetProcessDpiAwarenessContext");

    if (setProcessDpiAwarenessContext && setProcessDpiAwarenessContext((void *)-4)) /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2  */
    {
      return 1;
    }
  }

  shcore = LoadLibraryA("Shcore.dll");

  if (shcore)
  {
    typedef i32(__stdcall * SetProcessDpiAwarenessProc)(i32);
    SetProcessDpiAwarenessProc setDpiAwareness;

    *(void **)(&setDpiAwareness) = GetProcAddress(shcore, "SetProcessDpiAwareness");

    if (setDpiAwareness)
    {
      setDpiAwareness(2); /* PROCESS_PER_MONITOR_DPI_AWARE */
    }

    FreeLibrary(shcore);
  }
  else
  {
    SetProcessDPIAware();
  }

  return 1;
}

ZAI_API ZAI_INLINE u8 win32_enable_high_resolution_timer(void)
{
  void *winmm = LoadLibraryA("Winmm.dll");

  if (winmm)
  {
    typedef u32(__stdcall * timeBeginPeriodProc)(u32);
    timeBeginPeriodProc timeBeginPeriod;

    u32 res = 0;

    *(void **)(&timeBeginPeriod) = GetProcAddress(winmm, "timeBeginPeriod");

    if (!timeBeginPeriod)
    {
      return 0;
    }

    res = timeBeginPeriod(1);

    FreeLibrary(winmm);

    /* TIMERR_NOCANDO */
    if (res == 97)
    {
      return 0;
    }
  }

  return 1;
}

ZAI_API i32 win32_process_thread_count(void)
{
  u32 pid = GetCurrentProcessId();
  void *snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  THREADENTRY32 te;
  i32 count = 0;

  if (snapshot == INVALID_HANDLE)
  {
    return 0;
  }

  te.dwSize = sizeof(te);

  if (Thread32First(snapshot, &te))
  {
    do
    {
      if (te.th32OwnerProcessID == pid)
      {
        count++;
      }
    } while (Thread32Next(snapshot, &te));
  }

  CloseHandle(snapshot);

  return count;
}

typedef struct process_memory_info
{
  u64 private_bytes; /* Commit charge (what you asked for) */
  u64 working_set;   /* RAM currently used */
  u64 peak_working_set;
} process_memory_info;

u8 win32_process_memory(process_memory_info *out)
{
  typedef i32(__stdcall * GetProcessMemoryInfo_Fn)(void *, PROCESS_MEMORY_COUNTERS_EX *, u32);
  static GetProcessMemoryInfo_Fn pGetMemInfo = 0;
  static i32 initialized = 0;

  PROCESS_MEMORY_COUNTERS_EX pmc;

  if (!initialized)
  {
    /* Win7+ */
    void *kernel32 = LoadLibraryA("kernel32.dll");

    if (kernel32)
    {
      *(void **)(&pGetMemInfo) = GetProcAddress(kernel32, "K32GetProcessMemoryInfo");
    }

    /* Vista and older */
    if (!pGetMemInfo)
    {
      void *psapi = LoadLibraryA("psapi.dll");

      if (psapi)
      {
        *(void **)(&pGetMemInfo) = GetProcAddress(psapi, "GetProcessMemoryInfo");
      }
    }

    initialized = 1;
  }

  if (!pGetMemInfo)
  {
    return 0;
  }

  pmc.cb = sizeof(pmc);

  if (!pGetMemInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
  {
    return 0;
  }

  out->private_bytes = (u64)pmc.PrivateUsage;
  out->working_set = (u64)pmc.WorkingSetSize;
  out->peak_working_set = (u64)pmc.PeakWorkingSetSize;

  return 1;
}

typedef struct win32_zai_application
{
  void *hDLL;
  FILETIME lastWriteTime;
  char *dllName;
} win32_zai_application;

typedef struct win32_zai_state
{
  win32_zai_application application;

  zai_platform_state platform_state;

  u32 window_width_pending;
  u32 window_height_pending;

  u8 controller_id;           /* The XInput id associated with this controller */
  u8 controller_check_needed; /* If a device is plugged in or disconnected we should check XInput controller state again */

  u8 shader_paused;
  u8 ui_enabled;
  u8 fullscreen_enabled;
  u8 borderless_enabled;
  u8 screen_recording_enabled;
  u8 screen_recording_initialized;

  void *window_handle;
  void *device_context;

  s8 *gl_version;
  s8 *gl_renderer;
  s8 *gl_vendor;
  i32 gl_max_3d_texture_size;

} win32_zai_state;

ZAI_API ZAI_INLINE i32 win32_vk_to_zai_key(i32 vk)
{
  if (vk >= '0' && vk <= '9')
  {
    return ZAI_KEYBOARD_KEY_0 + (vk - '0');
  }

  if (vk >= 'A' && vk <= 'Z')
  {
    return ZAI_KEYBOARD_KEY_A + (vk - 'A');
  }

  /* function keys (f1 - f12) */
  if (vk >= 0x70 && vk <= 0x7B)
  {
    return ZAI_KEYBOARD_KEY_F1 + (vk - 0x70);
  }

  switch (vk)
  {
  case 0x11:
    return ZAI_KEYBOARD_KEY_CONTROL;

  case 0x0D:
    return ZAI_KEYBOARD_KEY_RETURN;

  case 0x20:
    return ZAI_KEYBOARD_KEY_SPACE;

  case 0x10:
    return ZAI_KEYBOARD_KEY_SHIFT;

  case 0x09:
    return ZAI_KEYBOARD_KEY_TAB;

  case 0x12:
    return ZAI_KEYBOARD_KEY_ALT;

  case 0x1B:
    return ZAI_KEYBOARD_KEY_ESCAPE;

  case 0x25:
    return ZAI_KEYBOARD_KEY_LEFT;

  case 0x26:
    return ZAI_KEYBOARD_KEY_UP;

  case 0x27:
    return ZAI_KEYBOARD_KEY_RIGHT;

  case 0x28:
    return ZAI_KEYBOARD_KEY_DOWN;

  case 0xBB:
    return ZAI_KEYBOARD_KEY_PLUS;

  case 0xBD:
    return ZAI_KEYBOARD_KEY_MINUS;

  case 0xBC:
    return ZAI_KEYBOARD_KEY_COMMA;

  case 0xBE:
    return ZAI_KEYBOARD_KEY_POINT;
  }

  return -1;
}

ZAI_API ZAI_INLINE i64 win32_window_callback(void *window, u32 message, u64 wParam, i64 lParam)
{
  win32_zai_state *state = (win32_zai_state *)GetWindowLongPtrA(window, GWLP_USERDATA);

  i64 result = 0;

  switch (message)
  {
  case WM_ERASEBKGND:
    return 1;
  case WM_CREATE:
  {
    CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
    state = (win32_zai_state *)cs->lpCreateParams;
    SetWindowLongPtrA(window, GWLP_USERDATA, (i64)state);

    /* Setup raw input for mouse and keyboard */
    {
      RAWINPUTDEVICE rid[2] = {0};

      (void)RIDEV_INPUTSINK; /* Receive input even when not focused */

      rid[0].usUsagePage = 0x01;
      rid[0].usUsage = 0x06; /* Keyboard */
      rid[0].dwFlags = 0;    /* Receive input only when focused */
      rid[0].hwndTarget = window;

      rid[1].usUsagePage = 0x01;
      rid[1].usUsage = 0x02; /* Mouse */
      rid[1].dwFlags = 0;    /* Receive input only when focused */
      rid[1].hwndTarget = window;

      if (!RegisterRawInputDevices(rid, 2, sizeof(rid[0])))
      {
        win32_print("[win32] Failed to register RAWINPUT device\n");
      }
    }
  }
  break;
  case WM_CLOSE:
  case WM_QUIT:
  {
    if (!state)
    {
      break;
    }

    state->platform_state.running = 0;
  }
  break;
  case WM_SIZE:
  {
    if (!state)
    {
      break;
    }

    if (wParam == SIZE_MINIMIZED)
    {
      state->platform_state.window.minimized = 1;
    }
    else
    {
      state->platform_state.window.minimized = 0;
      state->platform_state.window.size_changed = 1;
      state->window_width_pending = (u16)(((u64)(lParam)) & 0xffff);          /* Low Word  */
      state->window_height_pending = (u16)((((u64)(lParam)) >> 16) & 0xffff); /* High Word */
    }
  }
  break;
  case WM_INPUT:
  {
    static u8 rawBuffer[128];
    RAWINPUT *raw = (RAWINPUT *)rawBuffer;

    u32 dwSize = 0;

    if (!state)
    {
      break;
    }

    GetRawInputData((RAWINPUT *)lParam, RID_INPUT, ZAI_NULL, &dwSize, sizeof(RAWINPUTHEADER));

    if (dwSize > sizeof(rawBuffer) ||
        GetRawInputData((RAWINPUT *)lParam, RID_INPUT, raw, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
    {
      return result;
    }

    if (raw->header.dwType == RIM_TYPEKEYBOARD)
    {
      RAWKEYBOARD *keyboard = &raw->data.keyboard;

      u16 vKey = keyboard->VKey;

      if (vKey < 256)
      {
        /*key->was_down = key->is_down;*/
        i32 zai_key = win32_vk_to_zai_key(vKey);

        state->platform_state.input.keyboard.keys_is_down[zai_key] = !(keyboard->Flags & RI_KEY_BREAK); /* 1 if pressed, 0 if released */
      }
    }
    else if (raw->header.dwType == RIM_TYPEMOUSE)
    {
      RAWMOUSE *mouse = &raw->data.mouse;

      i32 dx = mouse->lLastX;
      i32 dy = mouse->lLastY;

      state->platform_state.input.mouse.dx += dx;
      state->platform_state.input.mouse.dy -= dy;

      /* Scroll wheel */
      if (mouse->usButtonFlags & RI_MOUSE_WHEEL)
      {
        i16 wheelDelta = (i16)mouse->usButtonData;
        state->platform_state.input.mouse.scroll += (f32)wheelDelta / (f32)WHEEL_DELTA;
      }
    }
  }
  break;
  case WM_DEVICECHANGE:
  {
    /* Check if a device notification arrived in order to find out when to requery XInput controller states */
    /* DBT_DEVNODES_CHANGED is the most reliable for USB plugging/unplugging */
    if (wParam == DBT_DEVNODES_CHANGED || wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE)
    {
      state->controller_check_needed = 1;
    }
  }
  break;
  case WM_LBUTTONDOWN:
    state->platform_state.input.mouse.keys_is_down[ZAI_MOUSE_KEY_LEFT] = 1;
    break;
  case WM_LBUTTONUP:
    state->platform_state.input.mouse.keys_is_down[ZAI_MOUSE_KEY_LEFT] = 0;
    break;
  case WM_MBUTTONDOWN:
    state->platform_state.input.mouse.keys_is_down[ZAI_MOUSE_KEY_MIDDLE] = 1;
    break;
  case WM_MBUTTONUP:
    state->platform_state.input.mouse.keys_is_down[ZAI_MOUSE_KEY_MIDDLE] = 0;
    break;
  case WM_RBUTTONDOWN:
    state->platform_state.input.mouse.keys_is_down[ZAI_MOUSE_KEY_RIGHT] = 1;
    break;
  case WM_RBUTTONUP:
    state->platform_state.input.mouse.keys_is_down[ZAI_MOUSE_KEY_RIGHT] = 0;
    break;
  default:
  {
    result = DefWindowProcA(window, message, wParam, lParam);
  }
  break;
  }

  return (result);
}

static WINDOWPLACEMENT g_wpPrev = {0};

ZAI_API void win32_window_enter_fullscreen(win32_zai_state *state)
{
  i32 dwStyle = GetWindowLongA(state->window_handle, GWL_STYLE);

  if (dwStyle & WS_OVERLAPPEDWINDOW)
  {
    if (GetWindowPlacement(state->window_handle, &g_wpPrev))
    {
      MONITORINFO mi;
      mi.cbSize = sizeof(mi);

      GetMonitorInfoA(MonitorFromWindow(state->window_handle, MONITOR_DEFAULTTONEAREST), &mi);
      SetWindowLongA(state->window_handle, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);

      SetWindowPos(
          state->window_handle, HWND_TOP,
          mi.rcMonitor.left, mi.rcMonitor.top,
          mi.rcMonitor.right - mi.rcMonitor.left,
          mi.rcMonitor.bottom - mi.rcMonitor.top,
          SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

      state->window_width_pending = (u32)(mi.rcMonitor.right - mi.rcMonitor.left);
      state->window_height_pending = (u32)(mi.rcMonitor.bottom - mi.rcMonitor.top);
      state->platform_state.window.size_changed = 1;
    }
  }
}

ZAI_API void win32_window_enter_borderless(win32_zai_state *state)
{
  if (GetWindowPlacement(state->window_handle, &g_wpPrev))
  {
    i32 dwStyle = GetWindowLongA(state->window_handle, GWL_STYLE);

    MONITORINFO mi;
    mi.cbSize = sizeof(mi);

    GetMonitorInfoA(MonitorFromWindow(state->window_handle, MONITOR_DEFAULTTONEAREST), &mi);
    SetWindowLongA(state->window_handle, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);

    SetWindowPos(
        state->window_handle, HWND_TOP,
        mi.rcMonitor.left, mi.rcMonitor.top,
        mi.rcMonitor.right - mi.rcMonitor.left,
        mi.rcMonitor.bottom - mi.rcMonitor.top,
        SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    state->window_width_pending = (u32)(mi.rcMonitor.right - mi.rcMonitor.left);
    state->window_height_pending = (u32)(mi.rcMonitor.bottom - mi.rcMonitor.top);
    state->platform_state.window.size_changed = 1;
  }
}

ZAI_API void win32_window_enter_windowed(win32_zai_state *state)
{
  i32 dwStyle = GetWindowLongA(state->window_handle, GWL_STYLE);

  if (!(dwStyle & WS_OVERLAPPEDWINDOW))
  {
    RECT rect;

    SetWindowLongA(state->window_handle, GWL_STYLE, WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(state->window_handle, &g_wpPrev);
    SetWindowPos(state->window_handle, ZAI_NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    GetClientRect(state->window_handle, &rect);

    state->window_width_pending = (u32)(rect.right - rect.left);
    state->window_height_pending = (u32)(rect.bottom - rect.top);
    state->platform_state.window.size_changed = 1;
  }
  else
  {
    /* resize windowed mode */
    RECT rect = {0};
    rect.right = (i32)state->platform_state.window.width;
    rect.bottom = (i32)state->platform_state.window.height;

    AdjustWindowRect(&rect, (u32)GetWindowLongA(state->window_handle, GWL_STYLE), 0);
    SetWindowPos(state->window_handle, ZAI_NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  }
}

ZAI_API u8 win32_load_application(win32_zai_state *state)
{
  s8 *dllName = "zai.dll";
  s8 *dllTempName = "zai_temp.dll";

  win32_print("[win32] load application\n");

  if (state->application.hDLL != ZAI_NULL)
  {
    if (!FreeLibrary(state->application.hDLL))
    {
      win32_print("[win32] cannot free library: ");
      win32_print(dllTempName);
      win32_print("\n");
      return 0;
    }
    state->application.hDLL = ZAI_NULL;
  }

  if (!CopyFileA(dllName, dllTempName, 0))
  {
    win32_print("[win32] cannot copy file: ");
    win32_print(dllName);
    win32_print("\n");
    return 0;
  }

  state->application.hDLL = LoadLibraryA(dllTempName);
  state->application.dllName = dllName;
  state->application.lastWriteTime = win32_file_mod_time(dllName);

  if (!state->application.hDLL)
  {
    return 0;
  }

  /* FIX for ERROR: ISO C forbids conversion of object pointer to function pointer type*/
  /* https://pubs.opengroup.org/onlinepubs/009695399/functions/dlsym.html */
  *(void **)(&zai_update) = GetProcAddress(state->application.hDLL, "zai_update");

  return 1;
}

/* #############################################################################
 * # [SECTION] Font Loading and Parsing
 * #############################################################################
 */
ZAI_API void unpack_1bit_to_8bit(
    u8 *dst, /* width * height bytes */
    u8 *src, /* packed bits */
    u32 width,
    u32 height)
{
  u32 x;
  u32 y;

  for (y = 0; y < height; ++y)
  {
    for (x = 0; x < width; ++x)
    {
      u32 bit_index = y * width + x;
      u32 byte_index = bit_index >> 3;
      u32 bit = 7 - (bit_index & 7);
      u8 b = (src[byte_index] >> bit) & 1;

      /* 1 = black, 0 = white */
      dst[bit_index] = b ? 0xFF : 0x00;
    }
  }
}

typedef enum glyph_state_flag
{
  GLYPH_STATE_NONE = 0,
  GLYPH_STATE_BLINK = 1 << 8,
  GLYPH_STATE_VFLIP = 1 << 9,
  GLYPH_STATE_HFLIP = 1 << 10

} glyph_state_flag;

typedef struct glyph
{
  u16 x;
  u16 y;
  u16 packed_glyph_state; /* packs the glyph_index | glyph_state_flag */
  u16 packed_color;       /* rgb565_color */
} glyph;

ZAI_API u16 pack_rgb565(u8 r, u8 g, u8 b)
{
  return (u16)(((r >> 3) << 11) |
               ((g >> 2) << 5) |
               ((b >> 3) << 0));
}

ZAI_API ZAI_INLINE u16 glyph_advance_x(f32 font_scale)
{
  return (u16)((f32)(ZAI_FONT_GLYPH_WIDTH + 1) * font_scale);
}

ZAI_API ZAI_INLINE u16 glyph_advance_y(f32 font_scale)
{
  return (u16)(((f32)ZAI_FONT_GLYPH_HEIGHT * font_scale) + (f32)ZAI_FONT_GLYPH_HEIGHT);
}

ZAI_API void glyph_add(
    glyph *glyph_buffer,     /* pre allocated glyph array */
    u32 glyph_buffer_size,   /* maximum size */
    u32 *glyph_buffer_count, /* current size */
    s8 *text,                /* text to append */
    u16 *offset_x,           /* current offset x. Note: this is advanced by this function */
    u16 *offset_y,           /* current offset y. Note: this is advanced by this function */
    u16 color_rgb565,        /* rgb565 packed color. Use: pack_rgb565(255, 255, 255); */
    glyph_state_flag state,  /* GLYPH_STATE_BLINK | GLYPH_STATE_VFLIP | GLYPH_STATE_HFLIP */
    f32 font_scale)
{
  u16 start_x = *offset_x;

  u16 advance_x = glyph_advance_x(font_scale);
  u16 advance_y = glyph_advance_y(font_scale);

  while (*text && *glyph_buffer_count < glyph_buffer_size)
  {
    s8 c = *text++;
    i32 glyph_index;

    if (c == '\n')
    {
      *offset_x = start_x;
      *offset_y += advance_y;
      continue;
    }

    /* skip spaces explicitly */
    if (c == ' ')
    {
      *offset_x += advance_x;
      continue;
    }

    glyph_index = zai_font_s8_to_index(c);

    if (glyph_index < 0)
    {
      continue;
    }

    glyph_buffer[*glyph_buffer_count].x = *offset_x;
    glyph_buffer[*glyph_buffer_count].y = *offset_y;
    glyph_buffer[*glyph_buffer_count].packed_glyph_state = (u16)glyph_index | (u16)state;
    glyph_buffer[(*glyph_buffer_count)++].packed_color = color_rgb565;

    *offset_x += advance_x;
  }
}

/* #############################################################################
 * # [SECTION] OpenGL Context Creation and Shader Management
 * #############################################################################
 */
typedef struct shader_header
{
  u32 created;
  u32 program;
  u8 had_failure;

} shader_header;

typedef struct shader_font
{
  shader_header header;

  i32 loc_iResolution;
  i32 loc_iTime;
  i32 loc_iTextureInfo;
  i32 loc_iTexture;
  i32 loc_iFontScale;

} shader_font;

typedef struct shader_recording
{
  shader_header header;

  i32 loc_iResolution;
  i32 loc_iTime;

} shader_recording;

typedef struct shader_sky
{
  shader_header header;

  i32 loc_iResolution;
  i32 loc_iTime;
  i32 loc_camera;
  i32 loc_camera_basis;
  i32 loc_sun_dir;

} shader_sky;

typedef struct shader_marching_cubes
{
  shader_header header;

  i32 loc_iResolution;
  i32 loc_mvp;

} shader_marching_cubes;

typedef struct shader_terrain
{
  shader_header header;

  i32 loc_iResolution;
  i32 loc_camera;
  i32 loc_sun_dir;
  i32 loc_camera_view_dir;
  i32 loc_base_scale;
  i32 loc_mvp;
  i32 loc_texture_diffuse;
  i32 loc_texture_normal;
  i32 loc_texture_displacement;

} shader_terrain;

typedef struct shader_font_new
{
  shader_header header;

  i32 loc_iResolution;
  i32 loc_texture_font;

} shader_font_new;

typedef struct shader_tiles
{
  shader_header header;

  i32 loc_tile_offset;
  i32 loc_view_projection;
  i32 loc_is_dirty;

} shader_tiles;

typedef struct shader_main
{
  shader_header header;

  i32 loc_iResolution;
  i32 loc_iTime;
  i32 loc_iTimeDelta;
  i32 loc_iFrame;
  i32 loc_iFrameRate;
  i32 loc_iMouse;
  i32 loc_iTextureInfo;
  i32 loc_iTexture;
  i32 loc_iController;

  i32 loc_brick_map_texture;
  i32 loc_atlas_texture;
  i32 loc_material_texture;
  i32 loc_palette_texture;

  i32 loc_brick_map_dim;
  i32 loc_atlas_brick_dim;

  i32 loc_inverse_atlas_size;
  i32 loc_grid_start;
  i32 loc_cell_size;
  i32 loc_cell_diagonal;
  i32 loc_truncation;
  i32 loc_cell_size_inverse;

  /* Camera */
  i32 loc_camera_position;
  i32 loc_camera_forward;
  i32 loc_camera_right;
  i32 loc_camera_up;
  i32 loc_camera_forward_scaled;

} shader_main;

static u32 opengl_failed_function_load_count = 0;

ZAI_API PROC win32_opengl_load_function(s8 *name)
{
  static void *win32_opengl_lib = 0;

  PROC gl_function;

  if (!win32_opengl_lib)
  {
    win32_opengl_lib = LoadLibraryA("opengl32.dll");

    if (!win32_opengl_lib)
    {
      return ZAI_NULL;
    }
  }

  if (!wglGetProcAddress)
  {
    *(void **)(&wglGetProcAddress) = GetProcAddress(win32_opengl_lib, "wglGetProcAddress");

    if (!wglGetProcAddress)
    {
      win32_print("[opengl] FATAL: cannot load wglGetProcAddress!\n");
      FreeLibrary(win32_opengl_lib);
      win32_opengl_lib = ZAI_NULL;
      return ZAI_NULL;
    }
  }

  gl_function = wglGetProcAddress(name);

  /* Some GPU drivers do not return a valid null pointer if requested OpenGL function is not available */
  if (gl_function == (PROC)0 || gl_function == (PROC)0x1 || gl_function == (PROC)0x2 || gl_function == (PROC)0x3 || gl_function == (PROC)-1)
  {
    void *object_ptr = (void *)GetProcAddress(win32_opengl_lib, name);
    gl_function = *(PROC *)&object_ptr;
  }

  if (gl_function == (PROC)0)
  {
    win32_print("[opengl] \"");
    win32_print(name);
    win32_print("\" not found!\n");

    opengl_failed_function_load_count++;
  }

  return gl_function;
}

ZAI_API ZAI_INLINE i32 opengl_create_context(win32_zai_state *state)
{
  void *window_instance = GetModuleHandleA(0);
  WNDCLASSA window_class = {0};
  u32 window_style = WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
  RECT rect = {0};

  void *fake_window;
  void *fake_device_context;
  void *fake_rc;
  i32 fake_pixel_format;
  PIXELFORMATDESCRIPTOR fake_pfd = {0};

  ZAI_PROFILER_BEGIN(win32_opengl_init);

  window_class.style = CS_OWNDC;
  window_class.lpfnWndProc = win32_window_callback;
  window_class.hInstance = window_instance;
  window_class.hCursor = LoadCursorA(0, IDC_ARROW);
  window_class.hIcon = LoadIconA(window_instance, MAKEINTRESOURCEA(1));
  window_class.hbrBackground = 0;
  window_class.lpszClassName = state->platform_state.window.title;

  if (!RegisterClassA(&window_class))
  {
    return 0;
  }

  fake_window = CreateWindowExA(
      0,
      window_class.lpszClassName,
      window_class.lpszClassName,
      WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
      0, 0,
      1, 1,
      0, 0,
      window_instance, 0);

  if (!fake_window)
  {
    return 0;
  }

  fake_device_context = GetDC(fake_window);

  fake_pfd.nSize = sizeof(fake_pfd);
  fake_pfd.nVersion = 1;
  fake_pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  fake_pfd.iPixelType = PFD_TYPE_RGBA;
  fake_pfd.cColorBits = 32;
  fake_pfd.cAlphaBits = 8;
  fake_pfd.cDepthBits = 24;

  fake_pixel_format = ChoosePixelFormat(fake_device_context, &fake_pfd);

  if (!fake_pixel_format || !SetPixelFormat(fake_device_context, fake_pixel_format, &fake_pfd))
  {
    return 0;
  }

  win32_zai_opengl_load_functions(win32_opengl_load_function, 1);

  fake_rc = wglCreateContext(fake_device_context);

  if (!fake_rc || !wglMakeCurrent(fake_device_context, fake_rc))
  {
    return 0;
  }

  rect.right = (i32)state->platform_state.window.width;
  rect.bottom = (i32)state->platform_state.window.height;
  AdjustWindowRect(&rect, window_style, 0);

  state->window_handle = CreateWindowExA(
      0,
      window_class.lpszClassName,
      window_class.lpszClassName,
      window_style,
      0, 0,
      rect.right - rect.left,
      rect.bottom - rect.top,
      0, 0,
      window_instance,
      state /* Pass pointer to user data to the window callback */
  );

  /* Modal window */
  SetWindowPos(state->window_handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

  state->device_context = GetDC(state->window_handle);

  /* Load wgl function */
  win32_zai_opengl_load_functions(win32_opengl_load_function, 0);

  /* Load common opengl functions (platform independant) */
  zai_opengl_load_functions(win32_opengl_load_function);

  if (opengl_failed_function_load_count > 0)
  {
    win32_print("[opengl] Some of the required OpenGL functions are not available on your machine (see log file above)!\n");
    return 0;
  }

  /* Set Pixel Format */
  {

    i32 pixel_attributes[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_ALPHA_BITS_ARB, 8,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        0};

    i32 pixel_format_id;
    u32 num_formats;

    if (!wglChoosePixelFormatARB(state->device_context, pixel_attributes, 0, 1, &pixel_format_id, &num_formats) || !num_formats)
    {
      return 0;
    }

    SetPixelFormat(state->device_context, pixel_format_id, 0);
  }

  /* Create the OpenGL context */
  {
    i32 context_attributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0};

    void *rc = wglCreateContextAttribsARB(state->device_context, 0, context_attributes);

    wglMakeCurrent(0, 0);
    wglDeleteContext(fake_rc);
    ReleaseDC(fake_window, fake_device_context);
    DestroyWindow(fake_window);

    if (!rc || !wglMakeCurrent(state->device_context, rc))
    {
      return 0;
    }
  }

  /* Disable VSync */
  if (wglSwapIntervalEXT)
  {
    wglSwapIntervalEXT(0);
  }

  /* Print opengl information */
  state->gl_vendor = (s8 *)glGetString(GL_VENDOR);
  state->gl_renderer = (s8 *)glGetString(GL_RENDERER);
  state->gl_version = (s8 *)glGetString(GL_VERSION);

  win32_print("[opengl] vendor  : ");
  win32_print(state->gl_vendor);
  win32_print("\n");

  win32_print("[opengl] renderer: ");
  win32_print(state->gl_renderer);
  win32_print("\n");

  win32_print("[opengl] version : ");
  win32_print(state->gl_version);
  win32_print("\n");

  ZAI_PROFILER_END(win32_opengl_init);

  return 1;
}

ZAI_API u32 opengl_shader_load(shader_header *shader, s8 *shader_code_vertex, s8 *shader_code_fragment)
{
  u32 new_program;

  if (!zai_opengl_shader_create(&new_program, shader_code_vertex, shader_code_fragment, win32_print))
  {
    win32_print("[opengl] compile failed, keeping old shader is present\n");
    shader->had_failure = 1;
    return 0;
  }

  shader->had_failure = 0;

  /* If there has been already a shader created delete the old one */
  if (shader->created)
  {
    glDeleteProgram(shader->program);
  }

  shader->program = new_program;

  glUseProgram(shader->program);

  shader->created = 1;

  return 1;
}

ZAI_API void opengl_shader_load_shader_font(shader_font *shader)
{
  static s8 *shader_font_code_vertex =
      "#version 330 core\n"
      "layout(location=0)in vec2 p;"
      "layout(location=1)in uvec4 g;"
      "uniform vec3 r;"
      "uniform vec4 t;"
      "uniform float s;"
      "out vec2 vUV;"
      "out vec3 vC;"
      "flat out uint b;"
      "void main(){"
      "float i=g.z&255u,C=t.x/t.z;"
      "b=(g.z>>8)&1u;"
      "float v=(g.z>>9)&1u,h=(g.z>>10)&1u;"
      "vec2 Q=vec2(p.x*(1.-2.*h)+h,p.y*(1.-2.*v)+v);"
      "vec2 p2=vec2(g.xy)+Q*t.zw*s;"
      "vec2 N=p2/r.xy*2.-1.;"
      "gl_Position=vec4(N.x,-N.y,0,1);"
      "vUV=vec2((mod(i,C)+p.x)*t.z/t.x,(floor(i/C)+p.y)*t.w/t.y);"
      "uint K=g.w;"
      "vC=vec3((K>>11&31u)/31.,(K>>5&63u)/63.,(K&31u)/31.);"
      "}";

  static s8 *shader_font_code_fragment =
      "#version 330 core\n"
      "in vec2 vUV;"
      "in vec3 vC;"
      "flat in uint b;"
      "uniform sampler2D iTexture;"
      "uniform float iTime;"
      "out vec4 F;"
      "void main(){"
      "float a=texture(iTexture,vUV).r;"
      "if(b==1u&&fract(iTime)<.5)a=0.;" /* Blink Support */
      "F=vec4(vC,a);"
      "}";

  if (opengl_shader_load(&shader->header, shader_font_code_vertex, shader_font_code_fragment))
  {
    shader->loc_iResolution = glGetUniformLocation(shader->header.program, "r");
    shader->loc_iTextureInfo = glGetUniformLocation(shader->header.program, "t");
    shader->loc_iFontScale = glGetUniformLocation(shader->header.program, "s");
    shader->loc_iTime = glGetUniformLocation(shader->header.program, "iTime");
    shader->loc_iTexture = glGetUniformLocation(shader->header.program, "iTexture");
  }
}

static s8 *shader_code_vertex =
    "#version 330 core\n"
    "vec2 quad[3]=vec2[3]("
    "vec2(-1.0,-1.0),"
    "vec2(3.0,-1.0),"
    "vec2(-1.0,3.0)"
    ");"
    "void main(){gl_Position=vec4(quad[gl_VertexID],0.0,1.0);}";

ZAI_API void opengl_shader_load_shader_recording(shader_recording *shader)
{
  static s8 *shader_code_fragment =
      "#version 330 core\n"
      "out vec4 FragColor;"
      "uniform float iTime;"
      "uniform vec3 iRes;"
      "void main()"
      "{"
      "float r=10.0;"
      "float m=10.0;"
      "vec2 p=gl_FragCoord.xy;"
      "vec2 c=vec2(iRes.x-r-m, r+m);"
      "float d=length(p-c);"
      "float blink=0.5+0.5*sin(iTime*12.0);"
      "float b=4.0;"
      "if ((d<=r&&blink>0.5)||(p.x<b||p.y<b||p.x>=iRes.x-b||p.y>=iRes.y-b))"
      " FragColor=vec4(1.0,0.0,0.0,blink);"
      "else"
      " discard;"
      "}";

  if (opengl_shader_load(&shader->header, shader_code_vertex, shader_code_fragment))
  {
    shader->loc_iResolution = glGetUniformLocation(shader->header.program, "iRes");
    shader->loc_iTime = glGetUniformLocation(shader->header.program, "iTime");
  }
}

/* #############################################################################
 * # [SECTION] Test renderer prototype
 * #############################################################################
 */
ZAI_API ZAI_INLINE void zai_update_camera_movement(win32_zai_state *state, zai_camera *camera, f32 camera_speed)
{
  static u8 mouse_attached = 0;

  f32 cam_speed = camera_speed * (f32)state->platform_state.timing.time_delta;

  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_W])
  {
    camera->position = zai_vec3_add(camera->position, zai_vec3_mulf(camera->forward, cam_speed));
  }

  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_S])
  {
    camera->position = zai_vec3_sub(camera->position, zai_vec3_mulf(camera->forward, cam_speed));
  }

  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_A])
  {
    camera->position = zai_vec3_sub(camera->position, zai_vec3_mulf(camera->right, cam_speed));
  }

  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_D])
  {
    camera->position = zai_vec3_add(camera->position, zai_vec3_mulf(camera->right, cam_speed));
  }

  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_SPACE])
  {
    camera->position = zai_vec3_add(camera->position, zai_vec3_mulf(camera->worldUp, cam_speed));
  }

  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_CONTROL])
  {
    camera->position = zai_vec3_sub(camera->position, zai_vec3_mulf(camera->worldUp, cam_speed));
  }

  /* Roll handling */
  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_Q])
  {
    camera->roll -= 1.0f;
  }

  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_E])
  {
    camera->roll += 1.0f;
  }

  /* FOV handling */
  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_F] && !state->platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_F])
  {
    camera->fov = 90.0f;
    camera->roll = 0.0f;
  }

  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_PLUS])
  {
    camera->fov -= 1.0f;

    if (camera->fov <= 0.0f)
    {
      camera->fov = 1.0f;
    }
  }

  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_MINUS])
  {
    camera->fov += 1.0f;

    if (camera->fov >= 180.0f)
    {
      camera->fov = 179.0f;
    }
  }

  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_LEFT])
  {
    camera->yaw -= zai_minf(cam_speed * 0.1f, 89.0f);
  }

  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_RIGHT])
  {
    camera->yaw += zai_minf(cam_speed * 0.1f, 89.0f);
  }

  /* Mouse handling */
  if (state->platform_state.input.mouse.keys_is_down[ZAI_MOUSE_KEY_RIGHT] && !state->platform_state.input.mouse.keys_was_down[ZAI_MOUSE_KEY_RIGHT])
  {
    mouse_attached = !mouse_attached;
  }

  if (mouse_attached)
  {
    f32 mouseSensitivity = 0.1f;
    camera->yaw += zai_minf((f32)state->platform_state.input.mouse.dx * mouseSensitivity, 89.0f);
    camera->pitch += zai_maxf((f32)state->platform_state.input.mouse.dy * mouseSensitivity, -89.0f);
    camera->pitch = zai_clampf(camera->pitch, -89.0f, 89.0f);

    if (state->platform_state.input.mouse.scroll != 0.0f)
    {
      camera->fov = zai_clampf(camera->fov - (state->platform_state.input.mouse.scroll * 2), 1.0f, 179.0f);
    }
  }

  zai_camera_update(camera);
}

ZAI_API void zai_create_sdf_grid(win32_zai_state *state, zai_sparse_grid *grid, zai_vec3 grid_center, u32 grid_cell_count, f32 grid_cell_size)
{
  zai_sparse_grid_initialize(grid, grid_center, grid_cell_count, grid_cell_size);

  grid->brick_map_data = VirtualAlloc(0, grid->brick_map_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

  ZAI_PROFILER_BEGIN(sparse_grid_pass_01);
  zai_sparse_grid_pass_01_fill_brick_map(grid, zai_sdf_scene, state);
  ZAI_PROFILER_END(sparse_grid_pass_01);

  grid->atlas_data = VirtualAlloc(0, grid->atlas_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  grid->material_data = VirtualAlloc(0, grid->atlas_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

  /*
    state->mem_brick_map_bytes = grid->brick_map_bytes;
    state->mem_atlas_bytes = grid->atlas_bytes;
    state->grid_active_brick_count = grid->brick_map_active_bricks_count;
    state->grid_atlas_dimensions = grid->atlas_dimensions;
  */

  ZAI_PROFILER_BEGIN(sparse_grid_pass_02);
  zai_sparse_grid_pass_02_fill_atlas(grid, zai_sdf_scene, state);
  ZAI_PROFILER_END(sparse_grid_pass_02);
}

ZAI_API void zai_render_sdf_grid(win32_zai_state *state, zai_camera *camera)
{
  static u8 grid_initialized = 0;
  static shader_main main_shader = {0};
  static u32 main_vao;
  static zai_sparse_grid grid_lod0 = {0};
  static u32 brickMapTex;
  static u32 atlasTex;
  static u32 materialTex;
  static u32 paletteTex;
  static f32 cell_size_inverse = 0.0f;

  /* Camera */
  static zai_vec3 camera_forward_scaled;

  if (!grid_initialized)
  {
    u32 grid_cell_count = 128;
    f32 grid_cell_size = 1.0f / 16.0f;

    u32 size_code_vertex = 0;
    u32 size_code_fragment = 0;
    u8 *shader_code_vertex = win32_file_read("zai_font.vs", &size_code_vertex);
    u8 *shader_code_fragment = win32_file_read("zai.fs", &size_code_fragment);

    if (!shader_code_vertex || !shader_code_fragment || size_code_vertex < 1 || size_code_fragment < 1)
    {
      win32_print("Cannot load sdf shader files!\n");
      return;
    }

    if (opengl_shader_load(&main_shader.header, (s8 *)shader_code_vertex, (s8 *)shader_code_fragment))
    {
      main_shader.loc_iResolution = glGetUniformLocation(main_shader.header.program, "iResolution");
      main_shader.loc_iTime = glGetUniformLocation(main_shader.header.program, "iTime");
      main_shader.loc_iTimeDelta = glGetUniformLocation(main_shader.header.program, "iTimeDelta");
      main_shader.loc_iFrame = glGetUniformLocation(main_shader.header.program, "iFrame");
      main_shader.loc_iFrameRate = glGetUniformLocation(main_shader.header.program, "iFrameRate");
      main_shader.loc_iMouse = glGetUniformLocation(main_shader.header.program, "iMouse");
      main_shader.loc_iTextureInfo = glGetUniformLocation(main_shader.header.program, "iTextureInfo");
      main_shader.loc_iTexture = glGetUniformLocation(main_shader.header.program, "iTexture");
      main_shader.loc_iController = glGetUniformLocation(main_shader.header.program, "iController");

      main_shader.loc_brick_map_texture = glGetUniformLocation(main_shader.header.program, "uBrickMap");
      main_shader.loc_atlas_texture = glGetUniformLocation(main_shader.header.program, "uAtlas");
      main_shader.loc_material_texture = glGetUniformLocation(main_shader.header.program, "uMaterial");
      main_shader.loc_palette_texture = glGetUniformLocation(main_shader.header.program, "uPalette");

      main_shader.loc_brick_map_dim = glGetUniformLocation(main_shader.header.program, "uBrickMapDim");
      main_shader.loc_atlas_brick_dim = glGetUniformLocation(main_shader.header.program, "uAtlasBrickDim");
      main_shader.loc_inverse_atlas_size = glGetUniformLocation(main_shader.header.program, "uInvAtlasSize");
      main_shader.loc_grid_start = glGetUniformLocation(main_shader.header.program, "uGridStart");
      main_shader.loc_cell_size = glGetUniformLocation(main_shader.header.program, "uCellSize");
      main_shader.loc_cell_size_inverse = glGetUniformLocation(main_shader.header.program, "uInvCellSize");
      main_shader.loc_cell_diagonal = glGetUniformLocation(main_shader.header.program, "uCellDiagonal");
      main_shader.loc_truncation = glGetUniformLocation(main_shader.header.program, "uTruncation");

      /* Camera */
      main_shader.loc_camera_position = glGetUniformLocation(main_shader.header.program, "camera_position");
      main_shader.loc_camera_forward = glGetUniformLocation(main_shader.header.program, "camera_forward");
      main_shader.loc_camera_right = glGetUniformLocation(main_shader.header.program, "camera_right");
      main_shader.loc_camera_up = glGetUniformLocation(main_shader.header.program, "camera_up");
      main_shader.loc_camera_forward_scaled = glGetUniformLocation(main_shader.header.program, "camera_forward_scaled");
    }
    else
    {
      win32_print("Cannot compile sdf shaders!\n");
    }

    VirtualFree(shader_code_vertex, 0, MEM_RELEASE);
    VirtualFree(shader_code_fragment, 0, MEM_RELEASE);

    ZAI_PROFILER_BEGIN(sdf_scene_build);
    zai_sdf_scene_build();
    ZAI_PROFILER_END(sdf_scene_build);

    ZAI_PROFILER_BEGIN(sparse_grid_create_lod0);
    zai_create_sdf_grid(state, &grid_lod0, zai_vec3_zero, grid_cell_count, grid_cell_size); /* LOD 0 */
    ZAI_PROFILER_END(sparse_grid_create_lod0);

    cell_size_inverse = 1.0f / grid_lod0.cell_size;

    /* Generate a dummy vao with no buffer */
    glGenVertexArrays(1, &main_vao);
    glBindVertexArray(main_vao);

    /* Brick Map */
    glGenTextures(1, &brickMapTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_3D, brickMapTex);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_R16UI,
                 (i32)grid_lod0.brick_map_dimensions,
                 (i32)grid_lod0.brick_map_dimensions,
                 (i32)grid_lod0.brick_map_dimensions,
                 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, grid_lod0.brick_map_data);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    /* Atlas Texture */
    glGenTextures(1, &atlasTex);
    glBindTexture(GL_TEXTURE_3D, atlasTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8_SNORM,
                 (i32)grid_lod0.atlas_dimensions.x,
                 (i32)grid_lod0.atlas_dimensions.y,
                 (i32)grid_lod0.atlas_dimensions.z,
                 0, GL_RED, GL_BYTE, grid_lod0.atlas_data);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);

    /* Material Texture */
    (void)GL_R8UI;
    (void)GL_RED_INTEGER;
    (void)GL_NEAREST;

    glGenTextures(1, &materialTex);
    glBindTexture(GL_TEXTURE_3D, materialTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8,
                 (i32)grid_lod0.atlas_dimensions.x,
                 (i32)grid_lod0.atlas_dimensions.y,
                 (i32)grid_lod0.atlas_dimensions.z,
                 0, GL_RED, GL_UNSIGNED_BYTE, grid_lod0.material_data);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    /* Palette Texture */
    glGenTextures(1, &paletteTex);
    glBindTexture(GL_TEXTURE_1D, paletteTex);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB8, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, zai_sdf_scene_materials);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    grid_initialized = 1;
  }

  /* Camera Setup */
  camera_forward_scaled = zai_vec3_mulf(camera->forward, 1.5f);

  /******************************/
  /* Draw                       */
  /******************************/
  ZAI_PROFILER_BEGIN(gl_draw);

  glUseProgram(main_shader.header.program);

  /* General uniforms */
  glUniform3f(main_shader.loc_iResolution, (f32)state->platform_state.window.width, (f32)state->platform_state.window.height, 1.0f);
  glUniform1f(main_shader.loc_iTime, (f32)state->platform_state.timing.time_elapsed);

  /* Camera uniforms */
  glUniform3f(main_shader.loc_camera_position, camera->position.x, camera->position.y, camera->position.z);
  glUniform3f(main_shader.loc_camera_forward, camera->forward.x, camera->forward.y, camera->forward.z);
  glUniform3f(main_shader.loc_camera_right, camera->right.x, camera->right.y, camera->right.z);
  glUniform3f(main_shader.loc_camera_up, camera->up.x, camera->up.y, camera->up.z);
  glUniform3f(main_shader.loc_camera_forward_scaled, camera_forward_scaled.x, camera_forward_scaled.y, camera_forward_scaled.z);

  /* Grid uniforms */
  glUniform3f(main_shader.loc_brick_map_dim, (f32)grid_lod0.brick_map_dimensions * ZAI_BRICK_SIZE, (f32)grid_lod0.brick_map_dimensions * ZAI_BRICK_SIZE, (f32)grid_lod0.brick_map_dimensions * ZAI_BRICK_SIZE);
  glUniform3i(main_shader.loc_atlas_brick_dim, (i32)grid_lod0.atlas_bricks_per_row, (i32)0, (i32)0);
  glUniform3f(main_shader.loc_inverse_atlas_size, grid_lod0.atlas_dimensions_inverse.x, grid_lod0.atlas_dimensions_inverse.y, grid_lod0.atlas_dimensions_inverse.z);
  glUniform3f(main_shader.loc_grid_start, grid_lod0.start.x, grid_lod0.start.y, grid_lod0.start.z);
  glUniform1f(main_shader.loc_cell_size, grid_lod0.cell_size);
  glUniform3f(main_shader.loc_cell_size_inverse, cell_size_inverse, cell_size_inverse, cell_size_inverse);
  glUniform1f(main_shader.loc_truncation, grid_lod0.truncation_distance);

  /* Bind textures to texture units */
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, brickMapTex);
  glUniform1i(main_shader.loc_brick_map_texture, 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_3D, atlasTex);
  glUniform1i(main_shader.loc_atlas_texture, 1);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_3D, materialTex);
  glUniform1i(main_shader.loc_material_texture, 2);

  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_1D, paletteTex);
  glUniform1i(main_shader.loc_palette_texture, 3);

  glBindVertexArray(main_vao);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  ZAI_PROFILER_END(gl_draw);
}

ZAI_API void zai_render_sky(win32_zai_state *state, zai_camera *camera, zai_vec3 sun_dir, f32 camera_basis[9])
{
  static u8 sky_initialized = 0;
  static shader_sky sky_shader = {0};
  static u32 sky_vao;

  if (!sky_initialized)
  {
    u32 size_code_vertex = 0;
    u32 size_code_fragment = 0;
    u8 *shader_code_vertex = win32_file_read("zai_sky.vs", &size_code_vertex);
    u8 *shader_code_fragment = win32_file_read("zai_sky.fs", &size_code_fragment);

    if (!shader_code_vertex || !shader_code_fragment || size_code_vertex < 1 || size_code_fragment < 1)
    {
      win32_print("Cannot load sky shader files!\n");
      return;
    }

    if (opengl_shader_load(&sky_shader.header, (s8 *)shader_code_vertex, (s8 *)shader_code_fragment))
    {
      sky_shader.loc_iResolution = glGetUniformLocation(sky_shader.header.program, "iResolution");
      sky_shader.loc_iTime = glGetUniformLocation(sky_shader.header.program, "iTime");
      sky_shader.loc_camera = glGetUniformLocation(sky_shader.header.program, "cameraPos");
      sky_shader.loc_camera_basis = glGetUniformLocation(sky_shader.header.program, "cameraBasis");
      sky_shader.loc_sun_dir = glGetUniformLocation(sky_shader.header.program, "sunDir");
    }
    else
    {
      win32_print("Cannot compile shaders!\n");
    }

    VirtualFree(shader_code_vertex, 0, MEM_RELEASE);
    VirtualFree(shader_code_fragment, 0, MEM_RELEASE);

    glGenVertexArrays(1, &sky_vao);

    sky_initialized = 1;
  }

  /* Render */
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  glUseProgram(sky_shader.header.program);
  glBindVertexArray(sky_vao);

  glUniform3f(sky_shader.loc_iResolution, (f32)state->platform_state.window.width, (f32)state->platform_state.window.height, 1.0f);
  glUniform1f(sky_shader.loc_iTime, (f32)state->platform_state.timing.time_elapsed);
  glUniform3f(sky_shader.loc_sun_dir, sun_dir.x, sun_dir.y, sun_dir.z);
  glUniform3f(sky_shader.loc_camera, camera->position.x, camera->position.y, camera->position.z);
  glUniformMatrix3fv(sky_shader.loc_camera_basis, 1, GL_FALSE, camera_basis);

  glDrawArrays(GL_TRIANGLES, 0, 3);

  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
}

ZAI_API ZAI_INLINE void initialize_density_grid(f32 *grid, i32 grid_dimensions, f32 grid_total_size, zai_vec3 grid_center)
{
  i32 x, y, z;
  static f32 frequency = 0.03f;
  static f32 amplitude = 15.0f;
  static f32 lacunarity = 2.0f;
  static f32 gain = 0.5f;
  static f32 zai_noise_rotation[3][3] = {{0.00f, 0.80f, 0.60f}, {-0.80f, 0.36f, -0.48f}, {-0.60f, -0.48f, 0.64f}};
  static i32 octaves = 6;

  for (z = 0; z < grid_dimensions; ++z)
  {
    f32 wz = (((f32)z / ((f32)grid_dimensions - 1.0f)) - 0.5f) * grid_total_size + grid_center.z;

    for (y = 0; y < grid_dimensions; ++y)
    {
      f32 wy = (((f32)y / ((f32)grid_dimensions - 1.0f)) - 0.5f) * grid_total_size + grid_center.y;

      for (x = 0; x < grid_dimensions; ++x)
      {
        f32 wx = (((f32)x / ((f32)grid_dimensions - 1.0f)) - 0.5f) * grid_total_size + grid_center.x;

        f32 noise_val = zai_value_noise_3d_fbm_rotation(wx, wy, wz, frequency, octaves, lacunarity, gain, zai_noise_rotation);
        f32 offset = wy > 0.0f ? -wy * 0.6f : 0.0f;
        f32 final_density = (noise_val * amplitude) + offset;

        /*
        final_density = wy + 20.0f;
         */

        grid[z * grid_dimensions * grid_dimensions + y * grid_dimensions + x] = final_density;
      }
    }
  }
}

#define DIM 129
#define MAX_TRIANGLES (DIM * DIM * DIM * 5)
ZAI_API void zai_render_marching_cubes(win32_zai_state *state, zai_camera *camera)
{
  static u8 marching_cubes_initialized = 0;

  static f32 density_grid[DIM * DIM * DIM];
  static zai_marching_cubes_triangle *triangle_buffer;
  static zai_marching_cubes_triangle *triangle_buffer_1;
  static zai_marching_cubes_context ctx = {0};
  static zai_marching_cubes_context ctx_1 = {0};
  static i32 triangle_count = 0;
  static i32 triangle_count_1 = 0;
  static u32 vao;
  static u32 vao_1;
  static u32 vbo;
  static u32 vbo_1;
  static shader_marching_cubes marching_cubes_shader;

  if (!marching_cubes_initialized)
  {
    ZAI_PROFILER_BEGIN(setup_marching_cubes);

    zai_noise_seed(0xDEADBEEF); /* 1234 */

    /* Shader Setup */
    {
      u32 size_code_vertex = 0;
      u32 size_code_fragment = 0;
      u8 *shader_code_vertex = win32_file_read("zai_marching_cubes.vs", &size_code_vertex);
      u8 *shader_code_fragment = win32_file_read("zai_marching_cubes.fs", &size_code_fragment);

      if (!shader_code_vertex || !shader_code_fragment || size_code_vertex < 1 || size_code_fragment < 1)
      {
        win32_print("Cannot load marching cubes shader files!\n");
        return;
      }

      if (opengl_shader_load(&marching_cubes_shader.header, (s8 *)shader_code_vertex, (s8 *)shader_code_fragment))
      {
        marching_cubes_shader.loc_iResolution = glGetUniformLocation(marching_cubes_shader.header.program, "iResolution");
        marching_cubes_shader.loc_mvp = glGetUniformLocation(marching_cubes_shader.header.program, "MVP");

        if (marching_cubes_shader.loc_mvp < 0)
        {
          win32_print("Cannot find uniforms!\n");
        }
      }
      else
      {
        win32_print("Cannot compile shaders!\n");
      }
    }

    triangle_buffer = VirtualAlloc(0, sizeof(zai_marching_cubes_triangle) * MAX_TRIANGLES, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    triangle_buffer_1 = VirtualAlloc(0, sizeof(zai_marching_cubes_triangle) * MAX_TRIANGLES, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    /* LOD 0 */
    ctx.dim_size = DIM;
    ctx.grid_size = 100.0f; /* Total world-space size of the chunk */
    ctx.iso_level = 0.0f;   /* The "surface" is where density is 0 */
    ctx.chunk_coord.x = 0.0f;
    ctx.chunk_coord.y = 0.0f;
    ctx.chunk_coord.z = 0.0f;
    ctx.density_grid = density_grid;
    ctx.transition_mask = 0;
    ctx.lod_level = 0;

    ZAI_PROFILER_BEGIN(setup_density_grid);
    initialize_density_grid(density_grid, DIM, ctx.grid_size, ctx.chunk_coord);
    ZAI_PROFILER_END(setup_density_grid);

    ZAI_PROFILER_BEGIN(setup_triangles);
    zai_marching_cubes_generate(&ctx, triangle_buffer, &triangle_count);
    ZAI_PROFILER_END(setup_triangles);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, triangle_count * (i32)sizeof(zai_marching_cubes_triangle), triangle_buffer, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(zai_marching_cubes_vertex), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(zai_marching_cubes_vertex), (void *)(sizeof(f32) * 3));

    /* LOD 1 */
    ctx_1.dim_size = DIM;
    ctx_1.grid_size = 100.0f; /* Total world-space size of the chunk */
    ctx_1.iso_level = 0.0f;   /* The "surface" is where density is 0 */
    ctx_1.chunk_coord.x = 0.0f;
    ctx_1.chunk_coord.y = 0.0f;
    ctx_1.chunk_coord.z = -100.0f;
    ctx_1.density_grid = density_grid;
    ctx_1.transition_mask = 0;
    ctx_1.lod_level = 0;

    ZAI_PROFILER_BEGIN(setup_density_grid_1);
    initialize_density_grid(density_grid, DIM, ctx_1.grid_size, ctx_1.chunk_coord);
    ZAI_PROFILER_END(setup_density_grid_1);

    ZAI_PROFILER_BEGIN(setup_triangles_1);
    zai_marching_cubes_generate(&ctx_1, triangle_buffer_1, &triangle_count_1);
    ZAI_PROFILER_END(setup_triangles_1);

    glGenVertexArrays(1, &vao_1);
    glBindVertexArray(vao_1);
    glGenBuffers(1, &vbo_1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_1);
    glBufferData(GL_ARRAY_BUFFER, triangle_count_1 * (i32)sizeof(zai_marching_cubes_triangle), triangle_buffer_1, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(zai_marching_cubes_vertex), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(zai_marching_cubes_vertex), (void *)(sizeof(f32) * 3));

    ZAI_PROFILER_END(setup_marching_cubes);

    marching_cubes_initialized = 1;
  }

  /* Render */
  ZAI_PROFILER_BEGIN(render_marching_cubes);
  {
    static u8 wireframe_enabled = 0;

    if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_TAB] && !state->platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_TAB])
    {
      wireframe_enabled = !wireframe_enabled;
    }

    {
      zai_mat4x4 projection = zai_mat4x4_perspective(ZAI_DEG_TO_RAD(camera->fov), (f32)state->platform_state.window.width / (f32)state->platform_state.window.height, 0.1f, 20000.0f);
      zai_mat4x4 view = zai_mat4x4_look_at(camera->position, zai_vec3_add(camera->position, camera->forward), camera->up);
      zai_mat4x4 mvp = zai_mat4x4_mul(projection, view);

      glEnable(GL_DEPTH_TEST);
      /*
       glEnable(GL_CULL_FACE);
       glCullFace(GL_BACK);
       */
      glPolygonMode(GL_FRONT_AND_BACK, wireframe_enabled ? GL_LINE : GL_FILL);
      glUseProgram(marching_cubes_shader.header.program);
      glUniformMatrix4fv(marching_cubes_shader.loc_mvp, 1, GL_FALSE, mvp.e);
      glUniform3f(marching_cubes_shader.loc_iResolution, (f32)state->platform_state.window.width, (f32)state->platform_state.window.height, 1.0f);

      glBindVertexArray(vao);
      glDrawArrays(GL_TRIANGLES, 0, triangle_count * 3);

      glBindVertexArray(vao_1);
      glDrawArrays(GL_TRIANGLES, 0, triangle_count_1 * 3);

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glDisable(GL_DEPTH_TEST);
      /*
      glDisable(GL_CULL_FACE);
      */
    }
  }
  ZAI_PROFILER_END(render_marching_cubes);

  (void)state;
  (void)zai_marching_cubes_corner_index_a_from_edge;
  (void)zai_marching_cubes_corner_index_b_from_edge;
  (void)zai_marching_cubes_triangulation;
}

ZAI_API void zai_render_surface_nets(win32_zai_state *state, zai_camera *camera)
{
  static u8 surface_nets_initialized = 0;

  static shader_marching_cubes marching_cubes_shader = {0};

  static zai_surface_nets_context chunk_1 = {0};
  static zai_surface_nets_context chunk_2 = {0};

  /* Large scratch buffers for mesh data */
  static zai_surface_nets_vertex *temp_verts;
  static u32 *temp_indices;
  static i32 vertex_count = 0;
  static i32 index_count = 0;
  static u32 vao;
  static u32 vbo;
  static u32 ebo;

  static zai_surface_nets_vertex *temp_verts_1;
  static u32 *temp_indices_1;
  static i32 vertex_count_1 = 0;
  static i32 index_count_1 = 0;
  static u32 vao_1;
  static u32 vbo_1;
  static u32 ebo_1;

  if (!surface_nets_initialized)
  {
    f32 *density_grid;
    i32 *cell_indices; /* Required for vertex lookup */

    ZAI_PROFILER_BEGIN(setup_surface_nets);

    /* Shader Setup */
    {
      u32 size_code_vertex = 0;
      u32 size_code_fragment = 0;
      u8 *shader_code_vertex = win32_file_read("zai_surface_nets.vs", &size_code_vertex);
      u8 *shader_code_fragment = win32_file_read("zai_surface_nets.fs", &size_code_fragment);

      if (!shader_code_vertex || !shader_code_fragment || size_code_vertex < 1 || size_code_fragment < 1)
      {
        win32_print("Cannot load marching cubes shader files!\n");
        return;
      }

      if (opengl_shader_load(&marching_cubes_shader.header, (s8 *)shader_code_vertex, (s8 *)shader_code_fragment))
      {
        marching_cubes_shader.loc_iResolution = glGetUniformLocation(marching_cubes_shader.header.program, "iResolution");
        marching_cubes_shader.loc_mvp = glGetUniformLocation(marching_cubes_shader.header.program, "MVP");

        if (marching_cubes_shader.loc_mvp < 0)
        {
          win32_print("Cannot find uniforms!\n");
        }
      }
      else
      {
        win32_print("Cannot compile shaders!\n");
      }
    }

    /* Memory allocation for chunks */
    density_grid = VirtualAlloc(0, DIM * DIM * DIM * sizeof(f32), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    cell_indices = VirtualAlloc(0, DIM * DIM * DIM * sizeof(i32), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    temp_verts = VirtualAlloc(0, DIM * DIM * DIM * sizeof(zai_surface_nets_vertex), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    temp_indices = VirtualAlloc(0, DIM * DIM * DIM * sizeof(i32), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    temp_verts_1 = VirtualAlloc(0, DIM * DIM * DIM * sizeof(zai_surface_nets_vertex), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    temp_indices_1 = VirtualAlloc(0, DIM * DIM * DIM * sizeof(i32), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    /* Chunk 1 */
    chunk_1.grid_dimensions = DIM;    /* 129 */
    chunk_1.grid_total_size = 100.0f; /* Total world-space size of the chunk */
    chunk_1.grid_center.x = 0.0f;
    chunk_1.grid_center.y = 0.0f;
    chunk_1.grid_center.z = 0.0f;
    chunk_1.iso_level = 0.0f; /* The "surface" is where density is 0 */
    chunk_1.density_grid = density_grid;
    chunk_1.buffer_indices = cell_indices;

    ZAI_PROFILER_BEGIN(setup_density_grid);
    initialize_density_grid(density_grid, DIM, chunk_1.grid_total_size, chunk_1.grid_center);
    ZAI_PROFILER_END(setup_density_grid);

    ZAI_PROFILER_BEGIN(setup_surface_nets_mesh);
    zai_surface_nets_generate(&chunk_1, temp_verts, &vertex_count, temp_indices, &index_count);
    ZAI_PROFILER_END(setup_surface_nets_mesh);

    /* Chunk 2 */
    {
      f32 cell_size = chunk_1.grid_total_size / (f32)(DIM - 1);
      f32 mesh_stride = chunk_1.grid_total_size - cell_size;

      chunk_2.grid_dimensions = DIM;
      chunk_2.grid_total_size = 100.0f;
      chunk_2.grid_center.x = 0.0f;
      chunk_2.grid_center.y = 0.0f;
      chunk_2.grid_center.z = chunk_1.grid_center.z - mesh_stride;
      chunk_2.iso_level = 0.0f;
      chunk_2.density_grid = density_grid;
      chunk_2.buffer_indices = cell_indices;
    }

    ZAI_PROFILER_BEGIN(setup_density_grid_1);
    initialize_density_grid(density_grid, DIM, chunk_2.grid_total_size, chunk_2.grid_center);
    ZAI_PROFILER_END(setup_density_grid_1);

    ZAI_PROFILER_BEGIN(setup_surface_nets_mesh_1);
    zai_surface_nets_generate(&chunk_2, temp_verts_1, &vertex_count_1, temp_indices_1, &index_count_1);
    ZAI_PROFILER_END(setup_surface_nets_mesh_1);

    /* OpenGL Buffer Setup */
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * (i32)sizeof(zai_surface_nets_vertex), temp_verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * (i32)sizeof(u32), temp_indices, GL_STATIC_DRAW);

    /* Position: Attribute 0 */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(zai_surface_nets_vertex), (void *)0);
    glEnableVertexAttribArray(0);
    /* Normal: Attribute 1 */
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(zai_surface_nets_vertex), (void *)(sizeof(zai_vec3)));
    glEnableVertexAttribArray(1);

    /* OpenGL Buffer Setup */
    glGenVertexArrays(1, &vao_1);
    glGenBuffers(1, &vbo_1);
    glGenBuffers(1, &ebo_1);

    glBindVertexArray(vao_1);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_1);
    glBufferData(GL_ARRAY_BUFFER, vertex_count_1 * (i32)sizeof(zai_surface_nets_vertex), temp_verts_1, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_1);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count_1 * (i32)sizeof(u32), temp_indices_1, GL_STATIC_DRAW);

    /* Position: Attribute 0 */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(zai_surface_nets_vertex), (void *)0);
    glEnableVertexAttribArray(0);
    /* Normal: Attribute 1 */
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(zai_surface_nets_vertex), (void *)(sizeof(zai_vec3)));
    glEnableVertexAttribArray(1);

    surface_nets_initialized = 1;

    ZAI_PROFILER_END(setup_surface_nets);
  }

  /* Render */
  ZAI_PROFILER_BEGIN(render_surface_nets);
  {
    static u8 wireframe_enabled = 0;

    if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_TAB] && !state->platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_TAB]) /* TAB */
    {
      wireframe_enabled = !wireframe_enabled;
    }

    {
      zai_mat4x4 projection = zai_mat4x4_perspective(ZAI_DEG_TO_RAD(camera->fov), (f32)state->platform_state.window.width / (f32)state->platform_state.window.height, 0.1f, 20000.0f);
      zai_mat4x4 view = zai_mat4x4_look_at(camera->position, zai_vec3_add(camera->position, camera->forward), camera->up);
      zai_mat4x4 mvp = zai_mat4x4_mul(projection, view);

      glEnable(GL_DEPTH_TEST);
      /*
      glEnable(GL_CULL_FACE);
      glCullFace(GL_BACK);
       */

      glPolygonMode(GL_FRONT_AND_BACK, wireframe_enabled ? GL_LINE : GL_FILL);
      glUseProgram(marching_cubes_shader.header.program);
      glUniform3f(marching_cubes_shader.loc_iResolution, (f32)state->platform_state.window.width, (f32)state->platform_state.window.height, 1.0f);
      glUniformMatrix4fv(marching_cubes_shader.loc_mvp, 1, GL_FALSE, mvp.e);

      glBindVertexArray(vao);
      glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);

      glBindVertexArray(vao_1);
      glDrawElements(GL_TRIANGLES, index_count_1, GL_UNSIGNED_INT, 0);

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glDisable(GL_DEPTH_TEST);

      /*
      glDisable(GL_CULL_FACE);
       */
    }
  }
  ZAI_PROFILER_END(render_surface_nets);
}

/* 65 */
#define GRID_RES 129
#define MAX_INDICES ((GRID_RES - 1) * (GRID_RES - 1) * 6)

ZAI_API void zai_render_terrain(win32_zai_state *state, zai_camera *camera, zai_vec3 sun_dir, f32 camera_basis[9])
{
  static u8 terrain_initialized = 0;
  static shader_terrain terrain_shader = {0};

  static u16 gridIndices[MAX_INDICES];
  static i32 gridIndexCount = 0;

  static u32 terrain_vao;
  static u32 terrain_ibo;

  static u32 tex_diffuse;
  static u32 tex_normal;
  static u32 tex_displacement;

  static i32 terrain_lod_count = 10;
  static i32 terrain_base_scale = 128;

  if (!terrain_initialized)
  {
    u32 size_code_vertex = 0;
    u32 size_code_fragment = 0;
    u8 *shader_code_vertex = win32_file_read("zai_terrain.vs", &size_code_vertex);
    u8 *shader_code_fragment = win32_file_read("zai_terrain.fs", &size_code_fragment);

    ZAI_PROFILER_BEGIN(setup_terrain);

    if (!shader_code_vertex || !shader_code_fragment || size_code_vertex < 1 || size_code_fragment < 1)
    {
      win32_print("Cannot load terrain shader files!\n");
      return;
    }

    if (opengl_shader_load(&terrain_shader.header, (s8 *)shader_code_vertex, (s8 *)shader_code_fragment))
    {
      terrain_shader.loc_iResolution = glGetUniformLocation(terrain_shader.header.program, "iResolution");
      terrain_shader.loc_camera = glGetUniformLocation(terrain_shader.header.program, "iCamera");
      terrain_shader.loc_sun_dir = glGetUniformLocation(terrain_shader.header.program, "sunDir");
      terrain_shader.loc_camera_view_dir = glGetUniformLocation(terrain_shader.header.program, "iViewDir");
      terrain_shader.loc_base_scale = glGetUniformLocation(terrain_shader.header.program, "iBaseScale");
      terrain_shader.loc_mvp = glGetUniformLocation(terrain_shader.header.program, "MVP");
      terrain_shader.loc_texture_diffuse = glGetUniformLocation(terrain_shader.header.program, "tex_diffuse");
      terrain_shader.loc_texture_normal = glGetUniformLocation(terrain_shader.header.program, "tex_normal");
      terrain_shader.loc_texture_displacement = glGetUniformLocation(terrain_shader.header.program, "tex_displacement");
    }
    else
    {
      win32_print("Cannot compile shaders!\n");
    }

    VirtualFree(shader_code_vertex, 0, MEM_RELEASE);
    VirtualFree(shader_code_fragment, 0, MEM_RELEASE);

    /* Generate Grid */
    {
      gridIndexCount = zai_geometry_grid(GRID_RES, gridIndices);
    }

    glGenVertexArrays(1, &terrain_vao);
    glBindVertexArray(terrain_vao);

    glGenBuffers(1, &terrain_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gridIndices), gridIndices, GL_STATIC_DRAW);

    /* Diffuse */
    {
      u32 w = 1024;
      u32 h = 1024;

      u32 file_size = 0;
      u8 *file_contents = win32_file_read("assets/sandstone/sandstone_cracks_diff_1k.raw", &file_size);

      if (!file_contents || file_size < 1)
      {
        win32_print("Cannot read diffuse texture!\n");
      }

      glGenTextures(1, &tex_diffuse);
      glBindTexture(GL_TEXTURE_2D, tex_diffuse);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (i32)w, (i32)h, 0, GL_RGB, GL_UNSIGNED_BYTE, file_contents);
      glGenerateMipmap(GL_TEXTURE_2D);
      glActiveTexture(GL_TEXTURE0);
    }

    /* Normal */
    {
      u32 w = 1024;
      u32 h = 1024;

      u32 file_size = 0;
      u8 *file_contents = win32_file_read("assets/sandstone/sandstone_cracks_nor_gl_1k.raw", &file_size);

      if (!file_contents || file_size < 1)
      {
        win32_print("Cannot read normal texture!\n");
      }

      glGenTextures(1, &tex_normal);
      glBindTexture(GL_TEXTURE_2D, tex_normal);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (i32)w, (i32)h, 0, GL_RGB, GL_UNSIGNED_BYTE, file_contents);
      glGenerateMipmap(GL_TEXTURE_2D);
      glActiveTexture(GL_TEXTURE1);
    }

    /* Displacement */
    {
      u32 w = 1024;
      u32 h = 1024;

      u32 file_size = 0;
      u8 *file_contents = win32_file_read("assets/sandstone/sandstone_cracks_disp_1k.raw", &file_size);

      if (!file_contents || file_size < 1)
      {
        win32_print("Cannot read displacement texture!\n");
      }

      glGenTextures(1, &tex_displacement);
      glBindTexture(GL_TEXTURE_2D, tex_displacement);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (i32)w, (i32)h, 0, GL_RGB, GL_UNSIGNED_BYTE, file_contents);
      glGenerateMipmap(GL_TEXTURE_2D);
      glActiveTexture(GL_TEXTURE2);
    }

    terrain_initialized = 1;

    ZAI_PROFILER_END(setup_terrain);
  }

  /* Render */
  ZAI_PROFILER_BEGIN(render_terrain);
  {
    static u8 wireframe_enabled = 0;

    if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_TAB] && !state->platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_TAB])
    {
      wireframe_enabled = !wireframe_enabled;
    }

    (void)camera_basis;

    {
      zai_mat4x4 projection = zai_mat4x4_perspective(ZAI_DEG_TO_RAD(camera->fov), (f32)state->platform_state.window.width / (f32)state->platform_state.window.height, 0.1f, 64000.0f);
      zai_mat4x4 view = zai_mat4x4_look_at(camera->position, zai_vec3_add(camera->position, camera->forward), camera->up);
      zai_mat4x4 mvp = zai_mat4x4_mul(projection, view);

      glEnable(GL_DEPTH_TEST);
      glDepthMask(GL_TRUE);
      glDepthFunc(GL_LESS);

      glEnable(GL_CULL_FACE);
      glCullFace(GL_BACK);

      glUseProgram(terrain_shader.header.program);
      glUniform3f(terrain_shader.loc_iResolution, (f32)state->platform_state.window.width, (f32)state->platform_state.window.height, 1.0f);
      glUniform3f(terrain_shader.loc_camera, camera->position.x, camera->position.y, camera->position.z);
      glUniform3f(terrain_shader.loc_camera_view_dir, camera->forward.x, camera->forward.y, camera->forward.z);
      glUniform3f(terrain_shader.loc_sun_dir, sun_dir.x, sun_dir.y, sun_dir.z);

      glUniform1f(terrain_shader.loc_base_scale, (f32)terrain_base_scale);
      glUniformMatrix4fv(terrain_shader.loc_mvp, 1, GL_FALSE, mvp.e);
      glBindVertexArray(terrain_vao);

      glPolygonMode(GL_FRONT_AND_BACK, wireframe_enabled ? GL_LINE : GL_FILL);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, tex_diffuse);
      glUniform1i(terrain_shader.loc_texture_diffuse, 0);

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, tex_normal);
      glUniform1i(terrain_shader.loc_texture_normal, 1);

      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, tex_displacement);
      glUniform1i(terrain_shader.loc_texture_displacement, 2);

      glDrawElementsInstanced(GL_TRIANGLES, gridIndexCount, GL_UNSIGNED_SHORT, 0, terrain_lod_count);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
    }
  }
  ZAI_PROFILER_END(render_terrain);
}

ZAI_API void zai_render_tiles(win32_zai_state *state, zai_camera *camera)
{
  static u8 tiles_initialized = 0;
  static zai_tiles t = {0};
  static i32 camera_tile_x = 0;
  static i32 camera_tile_z = 0;

  static shader_tiles tiles_shader = {0};

  static u32 quad_vao;
  static u32 quad_vbo;

  if (!tiles_initialized)
  {
    /* Setup tiles */
    ZAI_PROFILER_BEGIN(tile_init);
    zai_tiles_init(&t, camera_tile_x, camera_tile_z);
    ZAI_PROFILER_END(tile_init);

    ZAI_PROFILER_BEGIN(tile_update);
    zai_tiles_update(&t, camera_tile_x, camera_tile_z);
    ZAI_PROFILER_END(tile_update);

    /* Setup shaders */
    {
      u32 size_code_vertex = 0;
      u32 size_code_fragment = 0;
      u8 *shader_code_vertex = win32_file_read("zai_tiles.vs", &size_code_vertex);
      u8 *shader_code_fragment = win32_file_read("zai_tiles.fs", &size_code_fragment);

      if (!shader_code_vertex || !shader_code_fragment || size_code_vertex < 1 || size_code_fragment < 1)
      {
        win32_print("Cannot load tiles shader files!\n");
        return;
      }

      if (opengl_shader_load(&tiles_shader.header, (s8 *)shader_code_vertex, (s8 *)shader_code_fragment))
      {
        tiles_shader.loc_tile_offset = glGetUniformLocation(tiles_shader.header.program, "u_tile_offset");
        tiles_shader.loc_view_projection = glGetUniformLocation(tiles_shader.header.program, "u_vp");
        tiles_shader.loc_is_dirty = glGetUniformLocation(tiles_shader.header.program, "u_is_dirty");
      }
      else
      {
        win32_print("Cannot compile tiles shaders!\n");
      }

      VirtualFree(shader_code_vertex, 0, MEM_RELEASE);
      VirtualFree(shader_code_fragment, 0, MEM_RELEASE);
    }

    /* Setup Buffer Objects  */
    {
      /*
      f32 quad_vertices[] = {
          0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
          0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};
      */

      f32 quad_vertices[] = {
          0.0f,
          0.0f,
          1.0f,
          0.0f,
          1.0f,
          -1.0f,
          0.0f,
          0.0f,
          1.0f,
          -1.0f,
          0.0f,
          -1.0f,
      };

      /*
      f32 quad_vertices[] = {
          -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f,
          -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f};
      */

      glGenVertexArrays(1, &quad_vao);
      glGenBuffers(1, &quad_vbo);

      glBindVertexArray(quad_vao);
      glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void *)0);
      glBindVertexArray(0);
    }

    tiles_initialized = 1;
  }

  (void)state;
  (void)camera;

  /* Update camera tile based on cmaera world position */
  camera_tile_x = (i32)zai_floorf(camera->position.x + 0.5f);
  camera_tile_z = (i32)zai_floorf(camera->position.z + 0.5f);

  /* Check for new dirty tiles */
  ZAI_PROFILER_BEGIN(tile_update);
  zai_tiles_update(&t, camera_tile_x, camera_tile_z);
  ZAI_PROFILER_END(tile_update);

  /* Process dirty tiles */
  ZAI_PROFILER_BEGIN(tile_process_dirty);
  {
    i32 updates_per_frame = 1;

    while (updates_per_frame > 0 && t.dirty_indices_count > 0)
    {
      u32 last_idx = t.dirty_indices_count - 1;
      u32 tile_idx = t.dirty_indices[last_idx];

      (void)tile_idx;

      t.dirty_indices_count--;
      updates_per_frame--;
    }
  }
  ZAI_PROFILER_END(tile_process_dirty);

  /* Render Tiles */
  ZAI_PROFILER_BEGIN(tile_render);
  {
    static u8 wireframe_enabled = 0;

    u32 i;

    zai_mat4x4 projection = zai_mat4x4_perspective(ZAI_DEG_TO_RAD(camera->fov), (f32)state->platform_state.window.width / (f32)state->platform_state.window.height, 0.1f, 64000.0f);
    zai_mat4x4 view = zai_mat4x4_look_at(camera->position, zai_vec3_add(camera->position, camera->forward), camera->up);
    zai_mat4x4 mvp = zai_mat4x4_mul(projection, view);

    if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_TAB] && !state->platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_TAB])
    {
      wireframe_enabled = !wireframe_enabled;
    }

    glUseProgram(tiles_shader.header.program);
    glUniformMatrix4fv(tiles_shader.loc_view_projection, 1, GL_FALSE, mvp.e);

    glBindVertexArray(quad_vao);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_enabled ? GL_LINE : GL_FILL);

    for (i = 0; i < ZAI_TILES_TOTAL; ++i)
    {
      u8 is_dirty = zai_tile_is_dirty(&t, i);

      glUniform3f(tiles_shader.loc_tile_offset, (f32)t.tile_x[i], (f32)t.tile_z[i], 0.0f);
      glUniform1i(tiles_shader.loc_is_dirty, (i32)is_dirty);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
  ZAI_PROFILER_END(tile_render);
}

ZAI_API void zai_render_font(win32_zai_state *state, zai_camera *camera)
{
  static u8 font_initialized = 0;

  static shader_font_new font_shader = {0};
  static u32 tex_font;
  static u32 font_vao;

  if (!font_initialized)
  {
    /* Shader */
    {
      u32 size_code_vertex = 0;
      u32 size_code_fragment = 0;
      u8 *shader_code_vertex = win32_file_read("zai_font.vs", &size_code_vertex);
      u8 *shader_code_fragment = win32_file_read("zai_font.fs", &size_code_fragment);

      if (!shader_code_vertex || !shader_code_fragment || size_code_vertex < 1 || size_code_fragment < 1)
      {
        win32_print("Cannot load font shader files!\n");
        return;
      }

      if (opengl_shader_load(&font_shader.header, (s8 *)shader_code_vertex, (s8 *)shader_code_fragment))
      {
        font_shader.loc_iResolution = glGetUniformLocation(font_shader.header.program, "iResolution");
        font_shader.loc_texture_font = glGetUniformLocation(font_shader.header.program, "tex_font");
      }
      else
      {
        win32_print("Cannot compile shaders!\n");
      }

      VirtualFree(shader_code_vertex, 0, MEM_RELEASE);
      VirtualFree(shader_code_fragment, 0, MEM_RELEASE);
    }

    /* Font texture */
    {
      u32 w = 1024;
      u32 h = 1024;

      u32 file_size = 0;
      u8 *file_contents = win32_file_read("assets/font/codepage12.raw", &file_size);

      if (!file_contents || file_size < 1)
      {
        win32_print("Cannot read font texture!\n");
      }

      if (file_size != w * h * 4)
      {
        win32_print("Font texture size mismatch!\n");
      }

      glGenTextures(1, &tex_font);
      glBindTexture(GL_TEXTURE_2D, tex_font);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (i32)w, (i32)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, file_contents);
      glGenerateMipmap(GL_TEXTURE_2D);
      glActiveTexture(GL_TEXTURE0);

      VirtualFree(file_contents, 0, MEM_RELEASE);
    }

    glGenVertexArrays(1, &font_vao);

    font_initialized = 1;
  }

  (void)state;
  (void)camera;

  /* Rendering */
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glUseProgram(font_shader.header.program);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex_font);
  glUniform3f(font_shader.loc_iResolution, (f32)state->platform_state.window.width, (f32)state->platform_state.window.height, 1.0f);
  glUniform1i(font_shader.loc_texture_font, 0);

  glBindVertexArray(font_vao);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);

  glDisable(GL_BLEND);
}

ZAI_API void zai_render_scene(win32_zai_state *state)
{
  static u8 scene_initialized = 0;
  static u8 active_scene = 0;

  static zai_camera camera = {0};
  static f32 camera_basis[9];
  static f32 camera_speed = 1000.0f;

  f32 sun_dir_x;
  f32 sun_dir_y;
  f32 sun_dir_z;

  if (!scene_initialized)
  {
    camera = zai_camera_init();
    camera.position.y = 600.0f;

    scene_initialized = 1;
  }

  if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_I] && !state->platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_I])
  {
    active_scene++;

    if (active_scene > 4)
    {
      active_scene = 0;
    }

    if (active_scene == 0)
    {
      /* Clipmap Terrain */
      camera = zai_camera_init();
      camera.position.y = 600.0f;

      camera_speed = 1000.0f;
    }
    else if (active_scene == 1)
    {
      /* Surface nets */
      camera = zai_camera_init();
      camera.position.y = 20.0f;
      camera.position.z = 80.0f;

      camera_speed = 25.0f;
    }
    else if (active_scene == 2)
    {
      /* Marching Cubes */
      camera = zai_camera_init();
      camera.position.y = 20.0f;
      camera.position.z = 80.0f;

      camera_speed = 25.0f;
    }
    else if (active_scene == 3)
    {
      /* SDF Grid */
      camera = zai_camera_init();
      camera.position.y = 1.0f;
      camera.position.z = 2.0f;

      camera_speed = 1.0f;
    }
    else
    {
      /* Tiles */
      camera = zai_camera_init();
      camera.position.y = 1.0f;

      camera_speed = 1.0f;
    }
  }

  /* Update camera */
  zai_update_camera_movement(state, &camera, camera_speed);

  camera_basis[0] = camera.right.x;
  camera_basis[1] = camera.right.y;
  camera_basis[2] = camera.right.z;
  camera_basis[3] = camera.up.x;
  camera_basis[4] = camera.up.y;
  camera_basis[5] = camera.up.z;
  camera_basis[6] = camera.forward.x;
  camera_basis[7] = camera.forward.y;
  camera_basis[8] = camera.forward.z;

  {

    static u8 move_sun = 0;
    static u8 move_sun_loop = 0;
    static f32 move_val = 0.0f;

    if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_M] && !state->platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_M])
    {
      move_sun = !move_sun;

      if (move_sun)
      {
        move_val = 0.0f;
        move_sun_loop = 0;
      }
    }

    sun_dir_x = 0.0f;
    sun_dir_y = 0.0f;
    sun_dir_z = -1.0f;

    if (move_sun)
    {

      f32 t = (f32)state->platform_state.timing.time_elapsed * 0.5f;
      f32 len;

      (void)t;

      if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_L] && !state->platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_L])
      {
        move_sun_loop = !move_sun_loop;
      }

      if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_UP])
      {
        move_val += (f32)state->platform_state.timing.time_delta;
      }

      if (state->platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_DOWN])
      {
        move_val -= (f32)state->platform_state.timing.time_delta;

        if (move_val < 0.0f)
        {
          move_val = 0.0f;
        }
      }

      sun_dir_x = 0.0f;
      sun_dir_y = zai_sinf(move_val);
      sun_dir_z = -zai_cosf(move_val);

      if (move_sun_loop)
      {
        sun_dir_x = 0.0f;
        sun_dir_y = zai_sinf(t * 0.025f);
        sun_dir_z = -zai_cosf(t * 0.025f);
      }

      /* TODO(nickscha): BUG in SSE2 scalar & linear algebra library */
      len = zai_sqrtf(sun_dir_x * sun_dir_x + sun_dir_y * sun_dir_y + sun_dir_z * sun_dir_z);

      sun_dir_x /= len;
      sun_dir_y /= len;
      sun_dir_z /= len;
    }
  }

  /* Render */
  if (active_scene == 0)
  {
    zai_render_sky(state, &camera, zai_vec3_init(sun_dir_x, sun_dir_y, sun_dir_z), camera_basis);
    zai_render_terrain(state, &camera, zai_vec3_init(sun_dir_x, sun_dir_y, sun_dir_z), camera_basis);
  }
  else if (active_scene == 1)
  {
    zai_render_sky(state, &camera, zai_vec3_init(sun_dir_x, sun_dir_y, sun_dir_z), camera_basis);
    zai_render_surface_nets(state, &camera);
  }
  else if (active_scene == 2)
  {
    zai_render_sky(state, &camera, zai_vec3_init(sun_dir_x, sun_dir_y, sun_dir_z), camera_basis);
    zai_render_marching_cubes(state, &camera);
  }
  else if (active_scene == 3)
  {
    zai_render_sdf_grid(state, &camera);
  }
  else
  {
    zai_render_sky(state, &camera, zai_vec3_init(sun_dir_x, sun_dir_y, sun_dir_z), camera_basis);
    zai_render_tiles(state, &camera);
  }

  /* Render text */
  zai_render_font(state, &camera);
}

/* #############################################################################
 * # [SECTION] Main Entry Point
 * #############################################################################
 */
ZAI_API i32 start(i32 argc, u8 **argv)
{
  /* Default fragment shader file name to load if no file is passed as an argument in cli */
  s8 *fragment_shader_file_name = (argv && argc > 1) ? (s8 *)argv[1] : "zai.fs";

  win32_zai_state state = {0};
  shader_font font_shader = {0};
  shader_recording recording_shader = {0};

  u32 main_vao;
  u32 font_vao;
  u32 glyph_vbo;

  u32 tex;

  state.platform_state.running = 1;
  state.platform_state.window.title = "zai v0.1 (F1=Debug UI, F2=Screen Recording, R=Reset, P=Pause, F9=Borderless, F11=Fullscreen)";
  state.platform_state.timing.frame_rate_target = 30; /* 60 FPS, 0 = unlimited */
  state.controller_check_needed = 1;                  /* By default we have to query first XInput state */

  /* Platform API setup */
  state.platform_state.api.io_print = win32_print;

  /* Platform Window setup */
  state.platform_state.window.width = 800;
  state.platform_state.window.height = 600;
  state.platform_state.window.clear_color_r = 0.1f;
  state.platform_state.window.clear_color_g = 0.1f;
  state.platform_state.window.clear_color_b = 0.1f;

  /* Platform Memory Setup */
  state.platform_state.memory.size = 1024 * 1024 * 64; /* 64 MB */
  state.platform_state.memory.data = VirtualAlloc(0, state.platform_state.memory.size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

  (void)win32_file_read;

  /******************************/
  /* Load application once      */
  /******************************/
  if (!win32_load_application(&state))
  {
    win32_print("[ERROR] Could not load application!\n");
    return 1;
  }

  /******************************/
  /* Set Process Priorities     */
  /******************************/
  if (!win32_enable_high_priority())
  {
    win32_print("[WARNING] Failed to set high priority process\n");
  }

  /******************************/
  /* Set DPI aware mode         */
  /******************************/
  if (!win32_enable_dpi_awareness())
  {
    win32_print("[WARNING] Cannot set DPI awareness\n");
  }

  /******************************/
  /* HighRes timer for Sleep(1) */
  /******************************/
  if (!win32_enable_high_resolution_timer())
  {
    win32_print("[WARNING] Cannot set win32 high resolution timer using Winmm.dll (timeBeginPeriod)\n");
  }

  /******************************/
  /* Load XInput Controller     */
  /******************************/
  if (!xinput_load())
  {
    win32_print("[WARNING] Could not load XInput (xinput1_4.dll, xinput1_3.dll or xinput9_1_0.dll)! XInput gamepads/controllers (e.g. XBOX) are not functional.\n");
  }

  /******************************/
  /* Window and OpenGL context  */
  /******************************/
  if (!opengl_create_context(&state))
  {
    win32_print("[ERROR] Could not create opengl context!\n");
    return 1;
  }

  glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &state.gl_max_3d_texture_size);

  /* Avoid clear color flickering */
  glViewport(0, 0, (i32)state.platform_state.window.width, (i32)state.platform_state.window.height);
  glClearColor(state.platform_state.window.clear_color_r, state.platform_state.window.clear_color_g, state.platform_state.window.clear_color_b, state.platform_state.window.clear_color_a);
  glDisable(GL_FRAMEBUFFER_SRGB);
  glDisable(GL_MULTISAMPLE);
  glClear(GL_COLOR_BUFFER_BIT);

  SwapBuffers(state.device_context);

  /* Make the window visible */
  ShowWindow(state.window_handle, SW_SHOW);

  /******************************/
  /* Initialize Shaders         */
  /******************************/
  {
    /* Generate a dummy vao with no buffer */
    glGenVertexArrays(1, &main_vao);
    glBindVertexArray(main_vao);

    /* Load Fragment Shader source code from file */
    win32_print("[opengl] load shader file: ");
    win32_print(fragment_shader_file_name);
    win32_print("\n");

    opengl_shader_load_shader_font(&font_shader);
    opengl_shader_load_shader_recording(&recording_shader);
  }

  /******************************/
  /* Initialize Font Texture    */
  /******************************/
  {
    /* Generate font texture */
    u8 zai_font_pixels[ZAI_FONT_WIDTH * ZAI_FONT_HEIGHT];

    /* OpenGL does not allow 1bit packed texture data so we convert each bit to 1 byte */
    unpack_1bit_to_8bit(zai_font_pixels, zai_font, ZAI_FONT_WIDTH, ZAI_FONT_HEIGHT);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ZAI_FONT_WIDTH, ZAI_FONT_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, zai_font_pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glActiveTexture(GL_TEXTURE0);
  }

  /******************************/
  /* Initialize Font Buffers    */
  /******************************/
  {
    static f32 quad_vertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f};

    u32 quad_vbo;

    glGenVertexArrays(1, &font_vao);
    glBindVertexArray(font_vao);

    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    /* pos (location = 0) */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), ZAI_NULL);
    glVertexAttribDivisor(0, 0);

    glGenBuffers(1, &glyph_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, glyph_vbo);

    /* iGlyph (location = 1) */
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 4, GL_UNSIGNED_SHORT, sizeof(glyph), ZAI_NULL);
    glVertexAttribDivisor(1, 1);
  }

  {
    i64 perf_freq;
    i64 time_start;
    i64 time_start_fps_cap;
    i64 time_last;

    i32 thread_count = win32_process_thread_count();

    FILETIME fs_last = win32_file_mod_time(fragment_shader_file_name);

    QueryPerformanceFrequency(&perf_freq);
    QueryPerformanceCounter(&time_start);
    QueryPerformanceCounter(&time_start_fps_cap);

    time_last = time_start;

    while (state.platform_state.running)
    {
      i64 time_now;

      /******************************/
      /* Timing                     */
      /******************************/
      {
        QueryPerformanceCounter(&time_now);

        state.platform_state.timing.time_delta = (f64)(time_now - time_last) / (f64)perf_freq;
        state.platform_state.timing.time_elapsed = (f64)(time_now - time_start) / (f64)perf_freq;

        time_last = time_now;

        if (state.platform_state.timing.time_delta > 0.0)
        {
          state.platform_state.timing.frame_rate = 1.0 / state.platform_state.timing.time_delta;
        }
      }

      /******************************/
      /* Idle when window minimized */
      /******************************/
      if (state.platform_state.window.minimized)
      {
        MSG msg;
        GetMessageA(&msg, 0, 0, 0);
        DispatchMessageA(&msg);
        continue;
      }

      /******************************/
      /* Hot Reload Application     */
      /******************************/
      {
        FILETIME ddlFtCurrent = win32_file_mod_time(state.application.dllName);

        if (CompareFileTime(&ddlFtCurrent, &state.application.lastWriteTime) != 0)
        {
          win32_load_application(&state);
          state.platform_state.application_initialized = 0;
        }
      }

      /******************************/
      /* Hot Reload Fragment Shader */
      /******************************/
      {
        FILETIME fs_now = win32_file_mod_time(fragment_shader_file_name);

        if (CompareFileTime(&fs_now, &fs_last) != 0)
        {
          /*opengl_shader_load_shader_main(&main_shader, fragment_shader_file_name);*/
          fs_last = fs_now;

          /* Reset iTime elapsed seconds on hot reload */
          QueryPerformanceCounter(&time_start);
          state.platform_state.timing.frame_count = 0;
        }
      }

      /******************************/
      /* Input Processing           */
      /******************************/
      {
        MSG message = {0};

        u64 *src = (u64 *)state.platform_state.input.keyboard.keys_is_down;
        u64 *dst = (u64 *)state.platform_state.input.keyboard.keys_was_down;

        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;

        state.platform_state.input.mouse.keys_was_down[ZAI_MOUSE_KEY_LEFT] = state.platform_state.input.mouse.keys_is_down[ZAI_MOUSE_KEY_LEFT];
        state.platform_state.input.mouse.keys_was_down[ZAI_MOUSE_KEY_MIDDLE] = state.platform_state.input.mouse.keys_is_down[ZAI_MOUSE_KEY_MIDDLE];
        state.platform_state.input.mouse.keys_was_down[ZAI_MOUSE_KEY_RIGHT] = state.platform_state.input.mouse.keys_is_down[ZAI_MOUSE_KEY_RIGHT];
        state.platform_state.input.mouse.dx = 0;
        state.platform_state.input.mouse.dy = 0;

        while (PeekMessageA(&message, state.window_handle, 0, 0, PM_REMOVE))
        {
          DispatchMessageA(&message);
        }
      }

      /* Get current frames mouse position */
      {
        POINT p;
        GetCursorPos(&p);
        ScreenToClient(state.window_handle, &p);

        state.platform_state.input.mouse.x = p.x;
        state.platform_state.input.mouse.y = (i32)state.platform_state.window.height - 1 - p.y;
      }

      /******************************/
      /* XInput Controller          */
      /******************************/
      if (XInputGetState)
      {

        /* If we recieve a WM_DEVICECHANGE message (hardware connected or disconnected to machine)
         * we need to check again the XInput Controller state.
         */
        if (state.controller_check_needed)
        {
          u8 i = 0;

          XINPUT_STATE xinput_state = {0};

          state.platform_state.input.controller.connected = 0;

          for (i = 0; i < XINPUT_USER_MAX_COUNT; ++i)
          {
            u32 result = XInputGetState(i, &xinput_state);

            if (result == 0)
            {
              state.controller_id = i;
              state.platform_state.input.controller.connected = 1;
              break;
            }
          }

          state.controller_check_needed = 0;
        }

        if (state.platform_state.input.controller.connected)
        {
          XINPUT_STATE xinput_state = {0};
          u32 result = XInputGetState(state.controller_id, &xinput_state);

          if (result == 0)
          {
            XINPUT_GAMEPAD *gp = &xinput_state.Gamepad;
            state.platform_state.input.controller.button_a = (gp->wButtons & XINPUT_GAMEPAD_A) ? 1 : 0;
            state.platform_state.input.controller.button_b = (gp->wButtons & XINPUT_GAMEPAD_B) ? 1 : 0;
            state.platform_state.input.controller.button_x = (gp->wButtons & XINPUT_GAMEPAD_X) ? 1 : 0;
            state.platform_state.input.controller.button_y = (gp->wButtons & XINPUT_GAMEPAD_Y) ? 1 : 0;
            state.platform_state.input.controller.shoulder_left = (gp->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? 1 : 0;
            state.platform_state.input.controller.shoulder_right = (gp->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 1 : 0;
            state.platform_state.input.controller.dpad_up = (gp->wButtons & XINPUT_GAMEPAD_DPAD_UP) ? 1 : 0;
            state.platform_state.input.controller.dpad_down = (gp->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) ? 1 : 0;
            state.platform_state.input.controller.dpad_left = (gp->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? 1 : 0;
            state.platform_state.input.controller.dpad_right = (gp->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? 1 : 0;
            state.platform_state.input.controller.start = (gp->wButtons & XINPUT_GAMEPAD_START) ? 1 : 0;
            state.platform_state.input.controller.back = (gp->wButtons & XINPUT_GAMEPAD_BACK) ? 1 : 0;
            state.platform_state.input.controller.stick_left = (gp->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) ? 1 : 0;
            state.platform_state.input.controller.stick_right = (gp->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) ? 1 : 0;
            state.platform_state.input.controller.trigger_left_value = xinput_process_trigger(gp->bLeftTrigger);
            state.platform_state.input.controller.trigger_right_value = xinput_process_trigger(gp->bRightTrigger);
            state.platform_state.input.controller.trigger_left = state.platform_state.input.controller.trigger_left_value > 0.0f ? 1 : 0;
            state.platform_state.input.controller.trigger_right = state.platform_state.input.controller.trigger_right_value > 0.0f ? 1 : 0;
            state.platform_state.input.controller.stick_left_x = xinput_process_thumbstick(gp->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            state.platform_state.input.controller.stick_left_y = xinput_process_thumbstick(gp->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            state.platform_state.input.controller.stick_right_x = xinput_process_thumbstick(gp->sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
            state.platform_state.input.controller.stick_right_y = xinput_process_thumbstick(gp->sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
          }
          else
          {
            state.controller_check_needed = 1;
            state.platform_state.input.controller.connected = 0;
          }
        }
      }

      /******************************/
      /* Full or Borderless (F9,F11)*/
      /******************************/
      if (state.platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_F9] && !state.platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_F9]) /* F9 */
      {
        state.borderless_enabled = !state.borderless_enabled;

        if (state.borderless_enabled && !state.fullscreen_enabled)
        {
          win32_window_enter_borderless(&state);
        }
        else
        {
          state.fullscreen_enabled = 0;
          state.borderless_enabled = 0;
          win32_window_enter_windowed(&state);
        }
      }

      if (state.platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_F11] && !state.platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_F11])
      {
        state.fullscreen_enabled = !state.fullscreen_enabled;

        if (state.fullscreen_enabled && !state.borderless_enabled)
        {
          win32_window_enter_fullscreen(&state);
        }
        else
        {
          state.fullscreen_enabled = 0;
          state.borderless_enabled = 0;
          win32_window_enter_windowed(&state);
        }
      }

      /******************************/
      /* Handle Window Size changes */
      /******************************/
      if (state.platform_state.window.size_changed && !state.screen_recording_enabled)
      {
        state.platform_state.window.width = state.window_width_pending;
        state.platform_state.window.height = state.window_height_pending;

        glViewport(0, 0, (i32)state.platform_state.window.width, (i32)state.platform_state.window.height);

        state.platform_state.window.size_changed = 0;
      }

      /******************************/
      /* Pause Shader (P)           */
      /******************************/
      if (state.platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_P] && !state.platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_P])
      {
        state.shader_paused = !state.shader_paused;
      }

      /******************************/
      /* Reset Timer (R)            */
      /******************************/
      if (state.platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_R] && !state.platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_R])
      {
        /* Reset iTime elapsed seconds on hot reload */
        QueryPerformanceCounter(&time_start);
        state.platform_state.timing.frame_count = 0;
      }

      /******************************/
      /* Main Application Logic     */
      /******************************/
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      zai_update(&state.platform_state);

      zai_render_scene(&state);

      if (state.platform_state.window.size_changed)
      {
        state.window_width_pending = state.platform_state.window.width;
        state.window_height_pending = state.platform_state.window.height;
      }

      if (state.platform_state.window.clear_color_changed)
      {
        glClearColor(state.platform_state.window.clear_color_r, state.platform_state.window.clear_color_g, state.platform_state.window.clear_color_b, state.platform_state.window.clear_color_a);
        state.platform_state.window.clear_color_changed = 0;
      }

      if (state.platform_state.window.title_changed)
      {
        SetWindowTextA(state.window_handle, state.platform_state.window.title);
      }

      /******************************/
      /* UI Rendering (F1 pressed)  */
      /******************************/
      if (state.platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_F1] && !state.platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_F1])
      {
        state.ui_enabled = !state.ui_enabled;
      }

      if (state.ui_enabled)
      {

#define GLYPH_BUFFER_SIZE 4192
        static glyph glyph_buffer[GLYPH_BUFFER_SIZE];
        static u32 glyph_buffer_count = 0; /* Total glyph count */
        static u32 glyph_count_static = 0; /* Static glyphs only need to be buffered once */
        static u16 offset_x = 0;
        static u16 offset_y = 0;
        static u8 glyph_initialized = 0;
        static s8 tmp[128];
        static u32 handle_count = 0;
        static process_memory_info mem = {0};
        static f32 font_scale = 2.0f;

        u16 default_color = pack_rgb565(255, 255, 255);

        u16 advance_x = glyph_advance_x(font_scale);
        u16 advance_y = glyph_advance_y(font_scale);
        u16 separator_start_col = 11;
        u16 text_start_col = separator_start_col + 2;
        u16 offset_x_start = 10;
        u16 offset_y_start = 10;

        zai_sb t = {0};
        t.size = sizeof(tmp);
        t.buffer = tmp;

        if (!glyph_initialized)
        {
          u32 i = 0;

          offset_x = offset_x_start;
          offset_y = offset_y_start;

          glyph_add(
              glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count,
              "STATE\n"
              "FPS\n"
              "FPS TARGET\n"
              "FPS RAW\n"
              "FRAME\n"
              "DELTA\n"
              "TIME\n"
              "MOUSE X/Y\n"
              "MOUSE DX/DY\n"
              "SIZE X/Y\n"
              "THREADS\n"
              "HANDLES\n"
              "MEM WORK\n"
              "MEM PEAK\n"
              "MEM COMMIT\n"
              "GL VENDOR\n"
              "GL RENDER\n"
              "GL VERSION\n"
              "CONTROLLER\n"
              "STICK L X/Y\n"
              "STICK R X/Y\n"
              "TRIGGER L/R\n"
              "BUTTONS\n"
              "GL ERROR FS\n",
              &offset_x, &offset_y, default_color, GLYPH_STATE_NONE, font_scale);

          offset_x = (u16)(offset_x_start + advance_x * separator_start_col);
          offset_y = offset_y_start;

          /* Vertical ":" separator*/
          for (i = 0; i < 24; ++i)
          {

            if (i == 15)
            {
              u16 offset_x_new = (u16)(offset_x_start + advance_x * text_start_col);
              glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, state.gl_vendor, &offset_x_new, &offset_y, default_color, GLYPH_STATE_NONE, font_scale);
            }
            else if (i == 16)
            {
              u16 offset_x_new = (u16)(offset_x_start + advance_x * text_start_col);
              glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, state.gl_renderer, &offset_x_new, &offset_y, default_color, GLYPH_STATE_NONE, font_scale);
            }
            else if (i == 17)
            {
              u16 offset_x_new = (u16)(offset_x_start + advance_x * text_start_col);
              glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, state.gl_version, &offset_x_new, &offset_y, default_color, GLYPH_STATE_NONE, font_scale);
            }

            glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, ":\n", &offset_x, &offset_y, default_color, GLYPH_STATE_NONE, font_scale);
          }

          glyph_count_static = glyph_buffer_count;
          glyph_initialized = 1;
        }

        offset_x = (u16)(offset_x_start + advance_x * text_start_col);
        offset_y = offset_y_start;

        glyph_buffer_count = glyph_count_static;

        if (state.borderless_enabled)
        {
          glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, "BORDERLESS ", &offset_x, &offset_y, default_color, GLYPH_STATE_NONE, font_scale);
        }
        else if (state.fullscreen_enabled)
        {
          glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, "FULLSCREEN ", &offset_x, &offset_y, default_color, GLYPH_STATE_NONE, font_scale);
        }
        else
        {
          glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, "WINDOWED ", &offset_x, &offset_y, default_color, GLYPH_STATE_NONE, font_scale);
        }
        if (state.shader_paused)
        {
          glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, "PAUSED ", &offset_x, &offset_y, default_color, GLYPH_STATE_NONE, font_scale);
        }
        if (state.screen_recording_enabled)
        {
          glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, "RECORDING ", &offset_x, &offset_y, pack_rgb565(255, 0, 0), GLYPH_STATE_BLINK, font_scale);
        }

#define CALC_GLYPH(txt, color)                                                                                                              \
  do                                                                                                                                        \
  {                                                                                                                                         \
    offset_x = (u16)(offset_x_start + advance_x * text_start_col);                                                                          \
    offset_y = (u16)(offset_y + advance_y);                                                                                                 \
    glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, txt.buffer, &offset_x, &offset_y, color, GLYPH_STATE_NONE, font_scale); \
    txt.length = 0;                                                                                                                         \
  } while (0)

        zai_sb_f64(&t, state.platform_state.timing.frame_rate, 2);
        CALC_GLYPH(t, default_color);

        /* Control Target FPS with arrow keys (0 = unlimited) */
        {
          u32 txt_length_temp = 0;

          if (state.platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_LEFT] &&
              !state.platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_LEFT])
          {
            state.platform_state.timing.frame_rate_target -= state.platform_state.timing.frame_rate_target < 10 ? state.platform_state.timing.frame_rate_target : 10;
          }

          if (state.platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_RIGHT] &&
              !state.platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_RIGHT])
          {
            state.platform_state.timing.frame_rate_target += 10;
          }

          offset_x = (u16)(offset_x_start + advance_x * (text_start_col - 1));
          offset_y = (u16)(offset_y + advance_y);
          glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, "<", &offset_x, &offset_y, pack_rgb565(255, 0, 0), GLYPH_STATE_BLINK, font_scale);
          offset_y = (u16)(offset_y - advance_y);

          zai_sb_f64(&t, state.platform_state.timing.frame_rate_target, 2);
          txt_length_temp = t.length;
          CALC_GLYPH(t, default_color);

          offset_x = (u16)(offset_x_start + advance_x * (text_start_col + txt_length_temp));
          glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, ">", &offset_x, &offset_y, pack_rgb565(0, 255, 0), GLYPH_STATE_BLINK | GLYPH_STATE_HFLIP, font_scale);
        }

        zai_sb_f64(&t, state.platform_state.timing.frame_rate_raw, 2);
        CALC_GLYPH(t, default_color);
        zai_sb_i32(&t, state.platform_state.timing.frame_count);
        CALC_GLYPH(t, default_color);
        zai_sb_f64(&t, state.platform_state.timing.time_delta, 6);
        CALC_GLYPH(t, default_color);
        zai_sb_f64(&t, state.platform_state.timing.time_elapsed, 6);
        CALC_GLYPH(t, default_color);
        zai_sb_i32(&t, state.platform_state.input.mouse.x);
        zai_sb_s8(&t, "/");
        zai_sb_i32(&t, state.platform_state.input.mouse.y);
        CALC_GLYPH(t, default_color);
        zai_sb_i32(&t, state.platform_state.input.mouse.dx);
        zai_sb_s8(&t, "/");
        zai_sb_i32(&t, state.platform_state.input.mouse.dy);
        CALC_GLYPH(t, default_color);
        zai_sb_i32(&t, (i32)state.platform_state.window.width);
        zai_sb_s8(&t, "/");
        zai_sb_i32(&t, (i32)state.platform_state.window.height);
        CALC_GLYPH(t, default_color);
        zai_sb_i32(&t, thread_count);
        CALC_GLYPH(t, default_color);

        GetProcessHandleCount(GetCurrentProcess(), &handle_count);
        zai_sb_i32(&t, (i32)handle_count);
        CALC_GLYPH(t, default_color);

        win32_process_memory(&mem);
        zai_sb_i32(&t, (i32)(mem.working_set / 1024));
        CALC_GLYPH(t, default_color);
        zai_sb_i32(&t, (i32)(mem.peak_working_set / 1024));
        CALC_GLYPH(t, default_color);
        zai_sb_i32(&t, (i32)(mem.private_bytes / 1024));
        CALC_GLYPH(t, default_color);

        offset_y = (u16)(offset_y + advance_y * 3);

        if (state.platform_state.input.controller.connected)
        {
          zai_sb_s8(&t, "CONNECTED");
          CALC_GLYPH(t, pack_rgb565(0, 255, 0));
          zai_sb_f64(&t, state.platform_state.input.controller.stick_left_x, 6);
          zai_sb_s8(&t, "/");
          zai_sb_f64(&t, state.platform_state.input.controller.stick_left_y, 6);
          CALC_GLYPH(t, default_color);
          zai_sb_f64(&t, state.platform_state.input.controller.stick_right_x, 6);
          zai_sb_s8(&t, "/");
          zai_sb_f64(&t, state.platform_state.input.controller.stick_right_y, 6);
          CALC_GLYPH(t, default_color);
          zai_sb_f64(&t, state.platform_state.input.controller.trigger_left_value, 6);
          zai_sb_s8(&t, "/");
          zai_sb_f64(&t, state.platform_state.input.controller.trigger_right_value, 6);
          CALC_GLYPH(t, default_color);

          zai_sb_s8(&t, "");

          /* clang-format off */
          if (state.platform_state.input.controller.button_a)       zai_sb_s8(&t, "A ");
          if (state.platform_state.input.controller.button_b)       zai_sb_s8(&t, "B ");
          if (state.platform_state.input.controller.button_x)       zai_sb_s8(&t, "X ");
          if (state.platform_state.input.controller.button_y)       zai_sb_s8(&t, "Y ");
          if (state.platform_state.input.controller.dpad_left)      zai_sb_s8(&t, "DLEFT ");
          if (state.platform_state.input.controller.dpad_right)     zai_sb_s8(&t, "DRIGHT ");
          if (state.platform_state.input.controller.dpad_up)        zai_sb_s8(&t, "DUP ");
          if (state.platform_state.input.controller.dpad_down)      zai_sb_s8(&t, "DDOWN ");
          if (state.platform_state.input.controller.stick_left)     zai_sb_s8(&t, "LSTICK ");
          if (state.platform_state.input.controller.stick_right)    zai_sb_s8(&t, "RSTICK ");
          if (state.platform_state.input.controller.shoulder_left)  zai_sb_s8(&t, "LSHOULDER ");
          if (state.platform_state.input.controller.shoulder_right) zai_sb_s8(&t, "RSHOULDER ");
          /* clang-format on */

          CALC_GLYPH(t, default_color);
        }
        else
        {
          zai_sb_s8(&t, "NOT FOUND");
          CALC_GLYPH(t, pack_rgb565(255, 165, 0));

          offset_y = (u16)(offset_y + advance_y * 4);
        }

        zai_sb_s8(&t, "NONE");
        CALC_GLYPH(t, pack_rgb565(0, 255, 0));

#undef CALC_GLYPH

        /* Show profiler entries */
        {
          u16 offset_memory_x = 300;
          u16 offset_memory_y = 10;
          u32 i;

          for (i = 0; i < zai_profiler_entries_count; ++i)
          {
            zai_profiler_entry entry = zai_profiler_entries[i];

            u16 x;
            u16 y;

            t.length = 0;
            zai_sb_s8_pad(&t, entry.name, 23, ' ', ZAI_SB_PAD_RIGHT);
            zai_sb_s8(&t, ": ");
            zai_sb_f64(&t, entry.time_ms_last, 4);
            zai_sb_s8(&t, "/");
            zai_sb_f64(&t, entry.time_ms_total / (f64)entry.counter, 4);
            zai_sb_s8(&t, "/");
            zai_sb_f64(&t, entry.time_ms_total, 4);
            zai_sb_s8(&t, "/");
            zai_sb_i32(&t, (i32)entry.counter);
            zai_sb_s8(&t, "\n");

            x = offset_memory_x - 1;
            y = offset_memory_y - 1;

            glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, t.buffer, &offset_memory_x, &offset_memory_y, pack_rgb565(40, 40, 40), GLYPH_STATE_NONE, font_scale);
            glyph_add(glyph_buffer, GLYPH_BUFFER_SIZE, &glyph_buffer_count, t.buffer, &x, &y, pack_rgb565(255, 255, 255), GLYPH_STATE_NONE, font_scale);
          }
        }

        (void)GL_DYNAMIC_DRAW;

        glUseProgram(font_shader.header.program);
        glUniform3f(font_shader.loc_iResolution, (f32)state.platform_state.window.width, (f32)state.platform_state.window.height, 1.0f);
        glUniform1f(font_shader.loc_iTime, (f32)state.platform_state.timing.time_elapsed);
        glUniform4f(font_shader.loc_iTextureInfo, ZAI_FONT_WIDTH, ZAI_FONT_HEIGHT, ZAI_FONT_GLYPH_WIDTH, ZAI_FONT_GLYPH_HEIGHT);
        glUniform1i(font_shader.loc_iTexture, 0);
        glUniform1f(font_shader.loc_iFontScale, font_scale);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindVertexArray(font_vao);
        glBindBuffer(GL_ARRAY_BUFFER, glyph_vbo);
        glBufferData(GL_ARRAY_BUFFER, (i32)(glyph_buffer_count * sizeof(glyph)), glyph_buffer, GL_STREAM_DRAW);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, glyph_buffer_count);
        glBindVertexArray(0);

        glDisable(GL_BLEND);
      }

      /******************************/
      /* Screen Recording (F2)      */
      /******************************/
      {
        static u8 *framebuffer;
        static void *video_file_handle;

        if (state.platform_state.input.keyboard.keys_is_down[ZAI_KEYBOARD_KEY_F2] && !state.platform_state.input.keyboard.keys_was_down[ZAI_KEYBOARD_KEY_F2])
        {
          state.screen_recording_enabled = !state.screen_recording_enabled;
        }

        if (state.screen_recording_enabled)
        {

          u32 written = 0;

          if (!state.screen_recording_initialized)
          {
            s8 buffer[128];
            zai_sb t = {0};

            t.size = sizeof(buffer);
            t.buffer = buffer;

            /* Create recording output file name.
             * Format:  zai_capture_<window_width>x<window_height>_<platform_state.timing.frame_rate_target>.raw
             * Example: zai_capture_800x600_60.raw
             */
            zai_sb_s8(&t, "zai_capture_");
            zai_sb_i32(&t, (i32)(state.platform_state.window.width));
            zai_sb_s8(&t, "x");
            zai_sb_i32(&t, (i32)(state.platform_state.window.height));
            zai_sb_s8(&t, "_");
            zai_sb_i32(&t, (i32)(state.platform_state.timing.frame_rate_target));
            zai_sb_s8(&t, ".raw");

            framebuffer = VirtualAlloc(0, state.platform_state.window.width * state.platform_state.window.height * 3, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            video_file_handle = CreateFileA(t.buffer, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

            glPixelStorei(GL_PACK_ALIGNMENT, 1);

            state.screen_recording_initialized = 1;
          }
          glReadPixels(0, 0, (i32)state.platform_state.window.width, (i32)state.platform_state.window.height, GL_RGB, GL_UNSIGNED_BYTE, framebuffer);

          WriteFile(video_file_handle, framebuffer, state.platform_state.window.width * state.platform_state.window.height * 3, &written, 0);

          /* Render recording indicator */
          glUseProgram(recording_shader.header.program);
          glUniform3f(recording_shader.loc_iResolution, (f32)state.platform_state.window.width, (f32)state.platform_state.window.height, 1.0f);
          glUniform1f(recording_shader.loc_iTime, (f32)state.platform_state.timing.time_elapsed);
          glBindVertexArray(main_vao);
          glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        else if (state.screen_recording_initialized)
        {
          VirtualFree(framebuffer, 0, MEM_RELEASE);
          CloseHandle(video_file_handle);
          state.screen_recording_initialized = 0;
        }
      }

      SwapBuffers(state.device_context);

      /* Measure RAW FPS (rendering without cap)*/
      {
        i64 time_render_now;
        QueryPerformanceCounter(&time_render_now);
        state.platform_state.timing.frame_rate_raw = 1.0 / ((f64)(time_render_now - time_last) / (f64)perf_freq);
      }

      /******************************/
      /* Frame Rate Limiting        */
      /******************************/
      if (state.platform_state.timing.frame_rate_target > 0)
      {
        i64 time_end;

        f64 frame_time;
        f64 remaining;
        f64 target_frame_time = 1.0 / (f64)state.platform_state.timing.frame_rate_target;

        QueryPerformanceCounter(&time_end);

        frame_time = (f64)(time_end - time_start_fps_cap) / (f64)perf_freq;
        remaining = target_frame_time - frame_time;

        if (remaining > 0.0)
        {
          /* Sleep most of it (milliseconds) */
          if (remaining > 0.0005)
          {
            u32 sleep_ms = (u32)((remaining - 0.00025) * 1000.0);

            if (sleep_ms > 0)
            {
              Sleep(sleep_ms);
            }
          }

          /* Spin for the rest */
          for (;;)
          {
            QueryPerformanceCounter(&time_end);

            frame_time = (f64)(time_end - time_start_fps_cap) / (f64)perf_freq;

            if (frame_time >= target_frame_time)
            {
              break;
            }
          }
        }

        /* Start timing next frame */
        time_start_fps_cap = time_end;
      }

      state.platform_state.timing.frame_count++;
    }
  }

  return 0;
}

/* #############################################################################
 * # [SECTION] nostdlib entry point
 * #############################################################################
 */
#ifdef __clang__
#elif __GNUC__
__attribute((externally_visible))
#endif
i32 WinMainCRTStartup(void)
{
  u8 *cmdline = (u8 *)GetCommandLineA();
  u8 *argv[8];
  i32 argc = 0;

  i32 return_code;

  /* Parse command line arguments into argv */
  while (*cmdline)
  {
    /* skip whitespace */
    while (*cmdline == ' ' || *cmdline == '\t')
    {
      cmdline++;
    }

    if (!*cmdline)
    {
      break;
    }

    if (argc < 9)
    {
      argv[argc++] = cmdline;
    }

    /* parse token (basic, no quote handling) */
    while (*cmdline && *cmdline != ' ' && *cmdline != '\t')
    {
      cmdline++;
    }

    if (*cmdline)
    {
      *cmdline++ = '\0';
    }
  }

  argv[argc] = (u8 *)0;

  /* Run the program and exit with return code */
  return_code = start(argc, argv);
  ExitProcess((u32)return_code);
  return return_code;
}

/*
   ------------------------------------------------------------------------------
   This software is available under 2 licenses -- choose whichever you prefer.
   ------------------------------------------------------------------------------
   ALTERNATIVE A - MIT License
   Copyright (c) 2026 nickscha
   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   ------------------------------------------------------------------------------
   ALTERNATIVE B - Public Domain (www.unlicense.org)
   This is free and unencumbered software released into the public domain.
   Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
   software, either in source code form or as a compiled binary, for any purpose,
   commercial or non-commercial, and by any means.
   In jurisdictions that recognize copyright laws, the author or authors of this
   software dedicate any and all copyright interest in the software to the public
   domain. We make this dedication for the benefit of the public at large and to
   the detriment of our heirs and successors. We intend this dedication to be an
   overt act of relinquishment in perpetuity of all present and future rights to
   this software under copyright law.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   ------------------------------------------------------------------------------
*/
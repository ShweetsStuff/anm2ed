#pragma once
#include <cstring>
#include <glm/ext/vector_int2.hpp>

namespace anm2ed::resource::icon
{
  constexpr auto SIZE_SMALL = glm::ivec2(64, 64);
  constexpr auto SIZE_NORMAL = glm::ivec2(128, 128);
  constexpr auto SIZE_LARGE = glm::ivec2(256, 256);
  constexpr auto SIZE_HUGE = glm::ivec2(512, 512);

  constexpr auto NONE_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M12 22C6.47715 22 2 17.5228 2 12C2 6.47715 6.47715 2 12 2C17.5228 2 22 6.47715 22 12C22 17.5228 17.5228 22 12 22ZM11 15V17H13V15H11ZM11 7V13H13V7H11Z"/></svg>
  )";

  constexpr auto FILE_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M21 9V20.9925C21 21.5511 20.5552 22 20.0066 22H3.9934C3.44495 22 3 21.556 3 21.0082V2.9918C3 2.45531 3.44694 2 3.99826 2H14V8C14 8.55228 14.4477 9 15 9H21ZM21 7H16V2.00318L21 7Z"/></svg>
  )";

  constexpr auto FOLDER_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M3 3C2.44772 3 2 3.44772 2 4V7H9.58579L12 4.58579L10.4142 3H3ZM14.4142 5L10.4142 9H2V20C2 20.5523 2.44772 21 3 21H21C21.5523 21 22 20.5523 22 20V6C22 5.44772 21.5523 5 21 5H14.4142Z"/></svg>
  )";

  constexpr auto CLOSE_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M11.9997 10.5865L16.9495 5.63672L18.3637 7.05093L13.4139 12.0007L18.3637 16.9504L16.9495 18.3646L11.9997 13.4149L7.04996 18.3646L5.63574 16.9504L10.5855 12.0007L5.63574 7.05093L7.04996 5.63672L11.9997 10.5865Z"/></svg>
  )";

  constexpr auto ROOT_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M15.382 4H22V6H16.618L9 21.2361L5.38197 14H2V12H6.61803L9 16.7639L15.382 4Z"/></svg>
  )";

  constexpr auto LAYER_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M20.0833 10.4999L21.2854 11.2212C21.5221 11.3633 21.5989 11.6704 21.4569 11.9072C21.4146 11.9776 21.3557 12.0365 21.2854 12.0787L11.9999 17.6499L2.71451 12.0787C2.47772 11.9366 2.40093 11.6295 2.54301 11.3927C2.58523 11.3223 2.64413 11.2634 2.71451 11.2212L3.9166 10.4999L11.9999 15.3499L20.0833 10.4999ZM20.0833 15.1999L21.2854 15.9212C21.5221 16.0633 21.5989 16.3704 21.4569 16.6072C21.4146 16.6776 21.3557 16.7365 21.2854 16.7787L12.5144 22.0412C12.1977 22.2313 11.8021 22.2313 11.4854 22.0412L2.71451 16.7787C2.47772 16.6366 2.40093 16.3295 2.54301 16.0927C2.58523 16.0223 2.64413 15.9634 2.71451 15.9212L3.9166 15.1999L11.9999 20.0499L20.0833 15.1999ZM12.5144 1.30864L21.2854 6.5712C21.5221 6.71327 21.5989 7.0204 21.4569 7.25719C21.4146 7.32757 21.3557 7.38647 21.2854 7.42869L11.9999 12.9999L2.71451 7.42869C2.47772 7.28662 2.40093 6.97949 2.54301 6.7427C2.58523 6.67232 2.64413 6.61343 2.71451 6.5712L11.4854 1.30864C11.8021 1.11864 12.1977 1.11864 12.5144 1.30864Z"/></svg>
  )";

  constexpr auto NULL_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M6 5C5.44772 5 5 5.44772 5 6C5 6.55228 5.44772 7 6 7C6.55228 7 7 6.55228 7 6C7 5.44772 6.55228 5 6 5ZM3 6C3 4.34315 4.34315 3 6 3C7.65685 3 9 4.34315 9 6C9 7.30622 8.16519 8.41746 7 8.82929V9C7 10.1046 7.89543 11 9 11H15C16.1046 11 17 10.1046 17 9V8.82929C15.8348 8.41746 15 7.30622 15 6C15 4.34315 16.3431 3 18 3C19.6569 3 21 4.34315 21 6C21 7.30622 20.1652 8.41746 19 8.82929V9C19 11.2091 17.2091 13 15 13H13V15.1707C14.1652 15.5825 15 16.6938 15 18C15 19.6569 13.6569 21 12 21C10.3431 21 9 19.6569 9 18C9 16.6938 9.83481 15.5825 11 15.1707V13H9C6.79086 13 5 11.2091 5 9V8.82929C3.83481 8.41746 3 7.30622 3 6ZM18 5C17.4477 5 17 5.44772 17 6C17 6.55228 17.4477 7 18 7C18.5523 7 19 6.55228 19 6C19 5.44772 18.5523 5 18 5ZM12 17C11.4477 17 11 17.4477 11 18C11 18.5523 11.4477 19 12 19C12.5523 19 13 18.5523 13 18C13 17.4477 12.5523 17 12 17Z"/></svg>
  )";

  constexpr auto TRIGGERS_DATA = R"(
<svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M3 3H12.382C12.7607 3 13.107 3.214 13.2764 3.55279L14 5H20C20.5523 5 21 5.44772 21 6V17C21 17.5523 20.5523 18 20 18H13.618C13.2393 18 12.893 17.786 12.7236 17.4472L12 16H5V22H3V3Z"/></svg>
  )";

  constexpr auto VISIBLE_DATA = R"(
<svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M1.18164 12C2.12215 6.87976 6.60812 3 12.0003 3C17.3924 3 21.8784 6.87976 22.8189 12C21.8784 17.1202 17.3924 21 12.0003 21C6.60812 21 2.12215 17.1202 1.18164 12ZM12.0003 17C14.7617 17 17.0003 14.7614 17.0003 12C17.0003 9.23858 14.7617 7 12.0003 7C9.23884 7 7.00026 9.23858 7.00026 12C7.00026 14.7614 9.23884 17 12.0003 17ZM12.0003 15C10.3434 15 9.00026 13.6569 9.00026 12C9.00026 10.3431 10.3434 9 12.0003 9C13.6571 9 15.0003 10.3431 15.0003 12C15.0003 13.6569 13.6571 15 12.0003 15Z"/></svg>
  )";

  constexpr auto INVISIBLE_DATA = R"(
<svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M4.52047 5.93457L1.39366 2.80777L2.80788 1.39355L22.6069 21.1925L21.1927 22.6068L17.8827 19.2968C16.1814 20.3755 14.1638 21.0002 12.0003 21.0002C6.60812 21.0002 2.12215 17.1204 1.18164 12.0002C1.61832 9.62282 2.81932 7.5129 4.52047 5.93457ZM14.7577 16.1718L13.2937 14.7078C12.902 14.8952 12.4634 15.0002 12.0003 15.0002C10.3434 15.0002 9.00026 13.657 9.00026 12.0002C9.00026 11.537 9.10522 11.0984 9.29263 10.7067L7.82866 9.24277C7.30514 10.0332 7.00026 10.9811 7.00026 12.0002C7.00026 14.7616 9.23884 17.0002 12.0003 17.0002C13.0193 17.0002 13.9672 16.6953 14.7577 16.1718ZM7.97446 3.76015C9.22127 3.26959 10.5793 3.00016 12.0003 3.00016C17.3924 3.00016 21.8784 6.87992 22.8189 12.0002C22.5067 13.6998 21.8038 15.2628 20.8068 16.5925L16.947 12.7327C16.9821 12.4936 17.0003 12.249 17.0003 12.0002C17.0003 9.23873 14.7617 7.00016 12.0003 7.00016C11.7514 7.00016 11.5068 7.01833 11.2677 7.05343L7.97446 3.76015Z"/></svg>
  )";

  constexpr auto SHOW_UNUSED_DATA = R"(
<svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M6.92893 0.514648L21.0711 14.6568L19.6569 16.071L15.834 12.2486L15 13.4999V21.9999H9V13.4999L4 5.99993H3V3.99993L7.585 3.99965L5.51472 1.92886L6.92893 0.514648ZM9.585 5.99965L6.4037 5.99993L11 12.8944V19.9999H13V12.8944L14.392 10.8066L9.585 5.99965ZM21 3.99993V5.99993H20L18.085 8.87193L16.643 7.42893L17.5963 5.99993H15.213L13.213 3.99993H21Z"/></svg>
)";

  constexpr auto HIDE_UNUSED_DATA = R"(
<svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M21 4V6H20L15 13.5V22H9V13.5L4 6H3V4H21ZM6.4037 6L11 12.8944V20H13V12.8944L17.5963 6H6.4037Z"/></svg>
)";

  constexpr auto SHOW_LAYERS_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M20.0833 10.4999L21.2854 11.2212C21.5221 11.3633 21.5989 11.6704 21.4569 11.9072C21.4146 11.9776 21.3557 12.0365 21.2854 12.0787L11.9999 17.6499L2.71451 12.0787C2.47772 11.9366 2.40093 11.6295 2.54301 11.3927C2.58523 11.3223 2.64413 11.2634 2.71451 11.2212L3.9166 10.4999L11.9999 15.3499L20.0833 10.4999ZM20.0833 15.1999L21.2854 15.9212C21.5221 16.0633 21.5989 16.3704 21.4569 16.6072C21.4146 16.6776 21.3557 16.7365 21.2854 16.7787L12.5144 22.0412C12.1977 22.2313 11.8021 22.2313 11.4854 22.0412L2.71451 16.7787C2.47772 16.6366 2.40093 16.3295 2.54301 16.0927C2.58523 16.0223 2.64413 15.9634 2.71451 15.9212L3.9166 15.1999L11.9999 20.0499L20.0833 15.1999ZM12.5144 1.30864L21.2854 6.5712C21.5221 6.71327 21.5989 7.0204 21.4569 7.25719C21.4146 7.32757 21.3557 7.38647 21.2854 7.42869L11.9999 12.9999L2.71451 7.42869C2.47772 7.28662 2.40093 6.97949 2.54301 6.7427C2.58523 6.67232 2.64413 6.61343 2.71451 6.5712L11.4854 1.30864C11.8021 1.11864 12.1977 1.11864 12.5144 1.30864Z"/></svg>
)";

  constexpr auto HIDE_LAYERS_DATA = R"(
<svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M20.0833 15.1999L21.2854 15.9212C21.5221 16.0633 21.5989 16.3704 21.4569 16.6072C21.4146 16.6776 21.3557 16.7365 21.2854 16.7787L12.5144 22.0412C12.1977 22.2313 11.8021 22.2313 11.4854 22.0412L2.71451 16.7787C2.47772 16.6366 2.40093 16.3295 2.54301 16.0927C2.58523 16.0223 2.64413 15.9634 2.71451 15.9212L3.9166 15.1999L11.9999 20.0499L20.0833 15.1999ZM20.0833 10.4999L21.2854 11.2212C21.5221 11.3633 21.5989 11.6704 21.4569 11.9072C21.4146 11.9776 21.3557 12.0365 21.2854 12.0787L11.9999 17.6499L2.71451 12.0787C2.47772 11.9366 2.40093 11.6295 2.54301 11.3927C2.58523 11.3223 2.64413 11.2634 2.71451 11.2212L3.9166 10.4999L11.9999 15.3499L20.0833 10.4999ZM12.5144 1.30864L21.2854 6.5712C21.5221 6.71327 21.5989 7.0204 21.4569 7.25719C21.4146 7.32757 21.3557 7.38647 21.2854 7.42869L11.9999 12.9999L2.71451 7.42869C2.47772 7.28662 2.40093 6.97949 2.54301 6.7427C2.58523 6.67232 2.64413 6.61343 2.71451 6.5712L11.4854 1.30864C11.8021 1.11864 12.1977 1.11864 12.5144 1.30864ZM11.9999 3.33233L5.88723 6.99995L11.9999 10.6676L18.1126 6.99995L11.9999 3.33233Z"/></svg>
)";

  constexpr auto SHOW_RECT_DATA = R"(
  <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"> <rect x="4" y="4" width="16" height="16" rx="0.5" stroke="#FFF" stroke-width="2" stroke-linejoin="round"/> <path d="M12 9.5v5M9.5 12h5" stroke="#FFF" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/> </svg>
  )";

  constexpr auto HIDE_RECT_DATA = R"(
  <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"> <rect x="4" y="4" width="16" height="16" rx="0.5" stroke="#FFF" stroke-width="2" stroke-linejoin="round"/> <path d="M12 9.5v5M9.5 12h5" stroke="#FFF" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/> <path d="M2 2L22 22" stroke="#FFF" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/> </svg>
  )";

  constexpr auto ANIMATION_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M5.99807 7L8.30747 3H11.9981L9.68867 7H5.99807ZM11.9981 7L14.3075 3H17.9981L15.6887 7H11.9981ZM17.9981 7L20.3075 3H21.0082C21.556 3 22 3.44495 22 3.9934V20.0066C22 20.5552 21.5447 21 21.0082 21H2.9918C2.44405 21 2 20.5551 2 20.0066V3.9934C2 3.44476 2.45531 3 2.9918 3H5.99807L4 6.46076V19H20V7H17.9981Z"/></svg>
  )";

  constexpr auto SPRITESHEET_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M12 2C17.5222 2 22 5.97778 22 10.8889C22 13.9556 19.5111 16.4444 16.4444 16.4444H14.4778C13.5556 16.4444 12.8111 17.1889 12.8111 18.1111C12.8111 18.5333 12.9778 18.9222 13.2333 19.2111C13.5 19.5111 13.6667 19.9 13.6667 20.3333C13.6667 21.2556 12.9 22 12 22C6.47778 22 2 17.5222 2 12C2 6.47778 6.47778 2 12 2ZM10.8111 18.1111C10.8111 16.0843 12.451 14.4444 14.4778 14.4444H16.4444C18.4065 14.4444 20 12.851 20 10.8889C20 7.1392 16.4677 4 12 4C7.58235 4 4 7.58235 4 12C4 16.19 7.2226 19.6285 11.324 19.9718C10.9948 19.4168 10.8111 18.7761 10.8111 18.1111ZM7.5 12C6.67157 12 6 11.3284 6 10.5C6 9.67157 6.67157 9 7.5 9C8.32843 9 9 9.67157 9 10.5C9 11.3284 8.32843 12 7.5 12ZM16.5 12C15.6716 12 15 11.3284 15 10.5C15 9.67157 15.6716 9 16.5 9C17.3284 9 18 9.67157 18 10.5C18 11.3284 17.3284 12 16.5 12ZM12 9C11.1716 9 10.5 8.32843 10.5 7.5C10.5 6.67157 11.1716 6 12 6C12.8284 6 13.5 6.67157 13.5 7.5C13.5 8.32843 12.8284 9 12 9Z"/></svg>
  )";

  constexpr auto EVENT_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M13 10H20L11 23V14H4L13 1V10Z"/></svg>
  )";

  constexpr auto PAN_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M12 22L8 18H16L12 22ZM12 2L16 6H8L12 2ZM12 14C10.8954 14 10 13.1046 10 12C10 10.8954 10.8954 10 12 10C13.1046 10 14 10.8954 14 12C14 13.1046 13.1046 14 12 14ZM2 12L6 8V16L2 12ZM22 12L18 16V8L22 12Z"/></svg>
  )";

  constexpr auto MOVE_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M18 11V8L22 12L18 16V13H13V18H16L12 22L8 18H11V13H6V16L2 12L6 8V11H11V6H8L12 2L16 6H13V11H18Z"/></svg>
  )";

  constexpr auto ROTATE_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M16 7H11C7.68629 7 5 9.68629 5 13C5 16.3137 7.68629 19 11 19H20V21H11C6.58172 21 3 17.4183 3 13C3 8.58172 6.58172 5 11 5H16V1L22 6L16 11V7Z"/></svg>
  )";

  constexpr auto SCALE_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M21 3H13.5L16.5429 6.04289L13.2929 9.29289L14.7071 10.7071L17.9571 7.45711L21 10.5V3ZM3 21H10.5L7.45711 17.9571L10.7071 14.7071L9.29289 13.2929L6.04289 16.5429L3 13.5V21Z"/></svg>
  )";

  constexpr auto CROP_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M15 17V19H6C5.44772 19 5 18.5523 5 18V7H2V5H5V2H7V17H15ZM17 22V7H9V5H18C18.5523 5 19 5.44772 19 6V17H22V19H19V22H17Z"/></svg>
  )";

  constexpr auto DRAW_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M12.8995 6.85453L17.1421 11.0972L7.24264 20.9967H3V16.754L12.8995 6.85453ZM14.3137 5.44032L16.435 3.319C16.8256 2.92848 17.4587 2.92848 17.8492 3.319L20.6777 6.14743C21.0682 6.53795 21.0682 7.17112 20.6777 7.56164L18.5563 9.68296L14.3137 5.44032Z"/></svg>
  )";

  constexpr auto ERASE_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M13.9999 18.9966H20.9999V20.9966H11.9999L8.00229 20.9991L1.51457 14.5113C1.12405 14.1208 1.12405 13.4877 1.51457 13.0971L12.1212 2.49053C12.5117 2.1 13.1449 2.1 13.5354 2.49053L21.3136 10.2687C21.7041 10.6592 21.7041 11.2924 21.3136 11.6829L13.9999 18.9966ZM15.6567 14.5113L19.1922 10.9758L12.8283 4.61185L9.29275 8.14738L15.6567 14.5113Z"/></svg>
  )";

  constexpr auto COLOR_PICKER_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M15.5355 2.80744C17.0976 1.24534 19.6303 1.24534 21.1924 2.80744C22.7545 4.36953 22.7545 6.90219 21.1924 8.46429L18.3638 11.2929L18.7175 11.6466C19.108 12.0371 19.108 12.6703 18.7175 13.0608C18.327 13.4513 17.6938 13.4513 17.3033 13.0608L16.9498 12.7073L10.7351 18.922C10.1767 19.4804 9.46547 19.861 8.6911 20.0159L6.93694 20.3667C6.54976 20.4442 6.19416 20.6345 5.91496 20.9137L4.92894 21.8997C4.53841 22.2902 3.90525 22.2902 3.51472 21.8997L2.10051 20.4855C1.70999 20.095 1.70999 19.4618 2.10051 19.0713L3.08653 18.0852C3.36574 17.806 3.55605 17.4504 3.63348 17.0633L3.98431 15.3091C4.13919 14.5347 4.51981 13.8235 5.07821 13.2651L11.2929 7.05045L10.9393 6.69686C10.5488 6.30634 10.5488 5.67317 10.9393 5.28265C11.3299 4.89212 11.963 4.89212 12.3535 5.28265L12.7069 5.63604L15.5355 2.80744ZM12.7071 8.46466L6.49242 14.6794C6.21322 14.9586 6.02291 15.3142 5.94548 15.7013L5.59464 17.4555C5.43977 18.2299 5.05915 18.9411 4.50075 19.4995C5.05915 18.9411 5.77035 18.5604 6.54471 18.4056L8.29887 18.0547C8.68605 17.9773 9.04165 17.787 9.32085 17.5078L15.5355 11.2931L12.7071 8.46466Z"/></svg>
  )";

  constexpr auto UNDO_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M19.0001 13.9999L19.0002 5.00003L17.0002 5L17.0001 11.9999L9.41406 12V6.58581L2.99986 13L9.41406 19.4142L9.41406 14L19.0001 13.9999Z"/></svg>
  )";

  constexpr auto REDO_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M4.99989 13.9999L4.99976 5.00003L6.99976 5L6.99986 11.9999L14.5859 12V6.58581L21.0001 13L14.5859 19.4142L14.5859 14L4.99989 13.9999Z"/></svg>
  )";

  constexpr auto TARGET_DATA = R"(
  <svg viewBox="0 0 24 24" fill="none" stroke="#FFF" stroke-width="0.75" xmlns="http://www.w3.org/2000/svg"> <circle cx="12" cy="12" r="2.8"/> <line x1="12" y1="5" x2="12" y2="19"/> <line x1="5" y1="12" x2="19" y2="12"/> </svg>
  )";

  constexpr auto TARGET_ALT_DATA = R"(
<svg viewBox="0 0 24 24" fill="none" stroke="#FFF" stroke-width="0.75" xmlns="http://www.w3.org/2000/svg"> <circle cx="12" cy="12" r="2.8"/> <circle cx="12" cy="12" r="0.3" fill="#FFF" stroke="none"/> </svg>
  )";

  constexpr auto INTERPOLATED_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"> <circle cx="12" cy="12" r="2.5"/> </svg>
  )";

  constexpr auto UNINTERPOLATED_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"> <rect x="9.5" y="9.5" width="5" height="5"/> </svg>
  )";

  constexpr auto PIVOT_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"><path d="M11 11V5H13V11H19V13H13V19H11V13H5V11H11Z"/></svg>
  )";

  constexpr auto TRIGGER_DATA = R"(
  <svg viewBox="0 0 24 24" fill="#FFF" xmlns="http://www.w3.org/2000/svg"> <path d="M11 5h2v10h-2V5Zm0 14h2v2h-2v-2Z"/> </svg>
  )";

  constexpr auto PLAYHEAD_DATA = R"(
  <svg viewBox="0 0 24 48" fill="#FFF" xmlns="http://www.w3.org/2000/svg"> <path d="M4 0H20V38L12 48L4 38V0Z"/> </svg>
  )";

#define SVG_LIST                                                                                                       \
  X(NONE, NONE_DATA, SIZE_SMALL)                                                                                       \
  X(FILE, FILE_DATA, SIZE_NORMAL)                                                                                      \
  X(FOLDER, FOLDER_DATA, SIZE_NORMAL)                                                                                  \
  X(CLOSE, CLOSE_DATA, SIZE_NORMAL)                                                                                    \
  X(ROOT, ROOT_DATA, SIZE_NORMAL)                                                                                      \
  X(LAYER, LAYER_DATA, SIZE_NORMAL)                                                                                    \
  X(NULL_, NULL_DATA, SIZE_NORMAL)                                                                                     \
  X(TRIGGERS, TRIGGERS_DATA, SIZE_NORMAL)                                                                              \
  X(VISIBLE, VISIBLE_DATA, SIZE_NORMAL)                                                                                \
  X(INVISIBLE, INVISIBLE_DATA, SIZE_NORMAL)                                                                            \
  X(SHOW_RECT, SHOW_RECT_DATA, SIZE_NORMAL)                                                                            \
  X(HIDE_RECT, HIDE_RECT_DATA, SIZE_NORMAL)                                                                            \
  X(SHOW_UNUSED, SHOW_UNUSED_DATA, SIZE_NORMAL)                                                                        \
  X(HIDE_UNUSED, HIDE_UNUSED_DATA, SIZE_NORMAL)                                                                        \
  X(SHOW_LAYERS, SHOW_LAYERS_DATA, SIZE_NORMAL)                                                                        \
  X(HIDE_LAYERS, HIDE_LAYERS_DATA, SIZE_NORMAL)                                                                        \
  X(PAN, PAN_DATA, SIZE_NORMAL)                                                                                        \
  X(MOVE, MOVE_DATA, SIZE_NORMAL)                                                                                      \
  X(ROTATE, ROTATE_DATA, SIZE_NORMAL)                                                                                  \
  X(SCALE, SCALE_DATA, SIZE_NORMAL)                                                                                    \
  X(CROP, CROP_DATA, SIZE_NORMAL)                                                                                      \
  X(DRAW, DRAW_DATA, SIZE_NORMAL)                                                                                      \
  X(ERASE, ERASE_DATA, SIZE_NORMAL)                                                                                    \
  X(COLOR_PICKER, COLOR_PICKER_DATA, SIZE_NORMAL)                                                                      \
  X(UNDO, UNDO_DATA, SIZE_NORMAL)                                                                                      \
  X(REDO, REDO_DATA, SIZE_NORMAL)                                                                                      \
  X(ANIMATION, ANIMATION_DATA, SIZE_NORMAL)                                                                            \
  X(SPRITESHEET, SPRITESHEET_DATA, SIZE_NORMAL)                                                                        \
  X(EVENT, EVENT_DATA, SIZE_NORMAL)                                                                                    \
  X(INTERPOLATED, INTERPOLATED_DATA, SIZE_NORMAL)                                                                      \
  X(UNINTERPOLATED, UNINTERPOLATED_DATA, SIZE_NORMAL)                                                                  \
  X(TRIGGER, TRIGGER_DATA, SIZE_NORMAL)                                                                                \
  X(PLAYHEAD, PLAYHEAD_DATA, SIZE_NORMAL)                                                                              \
  X(PIVOT, PIVOT_DATA, SIZE_NORMAL)                                                                                    \
  X(POINT, UNINTERPOLATED_DATA, SIZE_NORMAL)                                                                           \
  X(TARGET, TARGET_DATA, SIZE_HUGE)                                                                                    \
  X(TARGET_ALT, TARGET_ALT_DATA, SIZE_HUGE)

  enum Type
  {
#define X(name, data, size) name,
    SVG_LIST
#undef X
        COUNT
  };

  struct Info
  {
    const char* data{};
    size_t length{};
    glm::ivec2 size{};
  };

  const Info ICONS[COUNT] = {
#define X(name, data, size) {data, std::strlen(data) - 1, size},
      SVG_LIST
#undef X
  };
}
#pragma once

namespace anm2ed
{
#define SELECT_ENGLISH(symbol, english, russian, chinese, korean) english,
#define SELECT_RUSSIAN(symbol, english, russian, chinese, korean) russian,
#define SELECT_CHINESE(symbol, english, russian, chinese, korean) chinese,
#define SELECT_KOREAN(symbol, english, russian, chinese, korean) korean,

#define LANGUAGES(X)                                                                                                   \
  X(ENGLISH, "English", SELECT_ENGLISH)                                                                                \
  X(RUSSIAN, "Pусский (Russian)", SELECT_RUSSIAN)                                                                      \
  X(CHINESE, "中文 (Chinese)", SELECT_CHINESE)                                                                         \
  X(KOREAN, "한국어 (Korean)", SELECT_KOREAN)

  enum Language
  {
#define X(symbol, label, selector) symbol,
    LANGUAGES(X)
#undef X
        LANGUAGE_COUNT
  };

  static constexpr const char* LANGUAGE_STRINGS[LANGUAGE_COUNT] = {
#define X(symbol, label, selector) label,
      LANGUAGES(X)
#undef X
  };

  // clang-format off
#define STRINGS(X)                                                                                                     \
  X(STRING_UNDEFINED, "undefined", "undefined", "undefined", "undefined") \
  X(BASIC_ADD, "Add", "Добавить", "添加", "추가") \
  X(BASIC_AFTER, "After", "После", "后一帧", "후") \
  X(BASIC_ALPHA, "Alpha", "Прозрачность", "透明度", "불투명도") \
  X(BASIC_APPEND, "Append", "Добавить к концу", "添加", "추가") \
  X(BASIC_AT_FRAME, "At Frame", "На кадре", "触发帧", "트리거될 프레임") \
  X(BASIC_BEFORE, "Before", "До", "前一帧", "전") \
  X(BASIC_BORDER, "Border", "Границы", "边框", "경계선") \
  X(BASIC_CANCEL, "Cancel", "Отмена", "取消", "취소") \
  X(BASIC_COLOR, "Color", "Цвет", "颜色", "색상") \
  X(BASIC_COLOR_OFFSET, "Color Offset", "Смещение цвета", "颜色偏移", "색상 오프셋") \
  X(BASIC_CONFIRM, "Confirm", "Подтвердить", "确定", "확인") \
  X(BASIC_COPY, "Copy", "Копировать", "复制", "복사") \
  X(BASIC_CROP, "Crop", "Обрезать", "裁剪", "자르기") \
  X(BASIC_CUT, "Cut", "Вырезать", "剪切", "잘라내기") \
  X(BASIC_DEFAULT, "Default", "По умолчанию", "默认", "기본값") \
  X(BASIC_DUPLICATE, "Duplicate", "Дублировать", "拷贝", "복제") \
  X(BASIC_DURATION, "Duration", "Продолжительность", "时长", "유지 시간") \
  X(BASIC_ENABLED, "Enabled", "Включено", "开启", "활성화") \
  X(BASIC_EVENT, "Event", "Событие", "事件", "이벤트") \
  X(BASIC_FRAME, "Frame", "Кадр", "帧", "프레임") \
  X(BASIC_FRAMES, "Frames", "Кадры", "帧", "프레임") \
  X(BASIC_GRID, "Grid", "Сетка", "网格", "격자") \
  X(BASIC_ID, "ID", "ID", "ID", "ID") \
  X(BASIC_INDEX, "Index", "Индекс", "下标", "인덱스") \
  X(BASIC_INTERPOLATED, "Interpolated", "Интерполировано", "线性插值", "매끄럽게 연결") \
  X(BASIC_LAYER_ANIMATION, "Layer", "Слой", "动画层", "레이어") \
  X(BASIC_MODE, "Mode", "Режим", "模式", "모드") \
  X(BASIC_NAME, "Name", "Имя", "名字", "이름") \
  X(BASIC_NEW, "New", "Новый", "新建", "새 파일") \
  X(BASIC_NO, "No", "Нет", "否", "아니오") \
  X(BASIC_NONE, "None", "Ничего", "无", "없음") \
  X(BASIC_NULL_ANIMATION, "Null", "Нуль", "无", "Null") \
  X(BASIC_OFFSET, "Offset", "Смещение", "偏移", "오프셋") \
  X(BASIC_OPEN, "Open", "Открыть", "打开", "열기") \
  X(BASIC_PASTE, "Paste", "Вставить", "粘贴", "붙여넣기") \
  X(BASIC_PIVOT, "Pivot", "Точка вращения", "枢轴", "중심점") \
  X(BASIC_POSITION, "Position", "Позиция", "位置", "위치") \
  X(BASIC_RELOAD, "Reload", "Перезагрузить", "重新加载", "다시 불러오기") \
  X(BASIC_REMOVE, "Remove", "Удалить", "删除", "제거") \
  X(BASIC_REMOVE_UNUSED, "Remove Unused", "Удалить неизпользуемые", "删除未使用", "미사용 시트 제거") \
  X(BASIC_REPLACE, "Replace", "Заменить", "替换", "교체") \
  X(BASIC_ROOT, "Root", "Корень", "根", "Root") \
  X(BASIC_ROTATION, "Rotation", "Поворот", "旋转", "회전") \
  X(BASIC_SAVE, "Save", "Сохранить", "保存", "저장") \
  X(BASIC_SCALE, "Scale", "Масштаб", "缩放", "크기") \
  X(BASIC_SIZE, "Size", "Размер", "大小", "비율") \
  X(BASIC_SOUND, "Sound", "Звук", "声音", "사운드") \
  X(BASIC_TIME, "Time", "Время", "时间", "시간") \
  X(BASIC_TINT, "Tint", "Оттенок", "色调", "색조") \
  X(BASIC_TRIGGERS, "Triggers", "Триггер", "事件触发器", "트리거") \
  X(BASIC_VISIBLE, "Visible", "Видимый", "显示", "표시") \
  X(BASIC_YES, "Yes", "Да", "是", "예") \
  X(BASIC_ZOOM, "Zoom", "Масштаб", "缩放", "확대") \
  X(EDIT_ADD_ANIMATION, "Add Animation", "Добавить анимацию", "添加动画", "애니메이션 추가") \
  X(EDIT_ADD_EVENT, "Add Event", "Добавить событие", "添加事件", "이벤트 추가") \
  X(EDIT_ADD_ITEM, "Add Item", "Добавить предмет", "添加物品", "항목 추가") \
  X(EDIT_ADD_LAYER, "Add Layer", "Добавить слой", "添加动画层", "레이어 추가") \
  X(EDIT_ADD_NULL, "Add Null", "Добавить нуль", "添加Null", "Null 추가") \
  X(EDIT_ADD_SPRITESHEET, "Add Spritesheet", "Добавить спрайт-лист", "添加图集", "스프라이트 시트 추가") \
  X(EDIT_ADD_SOUND, "Add Sound", "Добавить звук", "添加声音", "사운드 추가") \
  X(EDIT_ANIMATION_LENGTH, "Animation Length", "Длина анимации", "动画时长", "애니메이션 길이") \
  X(EDIT_AUTHOR, "Author", "Автор", "制作者", "작성자") \
  X(EDIT_BAKE_FRAMES, "Bake Frames", "Запечь кадры", "烘培/提前渲染", "프레임 베이크") \
  X(EDIT_CHANGE_FRAME_PROPERTIES, "Change Frame Properties", "Изменить свойства кадров", "更改帧属性", "프레임 속성 변경") \
  X(EDIT_CUT_ANIMATIONS, "Cut Animation(s)", "Вырезать анимации", "剪切动画", "애니메이션 잘라내기") \
  X(EDIT_CUT_FRAMES, "Cut Frame(s)", "Вырезать кадры", "剪切多个/单个帧", "프레임 잘라내기") \
  X(EDIT_DEFAULT_ANIMATION, "Default Animation", "Анимация по умолчанию", "默认动画", "기본 애니메이션") \
  X(EDIT_DELETE_FRAMES, "Delete Frame(s)", "Удалить кадры", "删除多个/单个帧", "프레임 삭제하기") \
  X(EDIT_DRAW, "Draw", "Рисовать", "画笔", "그리기") \
  X(EDIT_DUPLICATE_ANIMATIONS, "Duplicate Animation(s)", "Дублировать кадры", "复制动画", "애니메이션 복제") \
  X(EDIT_ERASE, "Erase", "Стереть", "擦除", "지우기") \
  X(EDIT_EXTEND_FRAME, "Extend Frame", "Удлиннить кадр", "延长帧时长", "프레임 확장") \
  X(EDIT_FIT_ANIMATION_LENGTH, "Fit Animation Length", "Подогнать к длине анимации", "匹配动画时长", "애니메이션 길이 맞추기") \
  X(EDIT_FPS, "FPS", "FPS", "每秒帧数(FPS)", "FPS") \
  X(EDIT_FRAME_COLOR_OFFSET, "Frame Color Offset", "Смещение цвета кадра", "帧颜色偏移", "프레임 색상 오프셋") \
  X(EDIT_FRAME_CROP, "Frame Crop", "Обрезка кадра", "帧裁剪", "프레임 자르기") \
  X(EDIT_FRAME_DURATION, "Frame Duration", "Продолжительность кадра", "帧时长", "프레임 유지 시간") \
  X(EDIT_FRAME_FLIP_X, "Frame Flip X", "Отразить кадр по X", "X轴翻转", "프레임 수평 뒤집기") \
  X(EDIT_FRAME_FLIP_Y, "Frame Flip Y", "Отразить кадр по Y", "Y轴翻转", "프레임 수직 뒤집기") \
  X(EDIT_FRAME_INTERPOLATION, "Frame Interpolation", "Интерполяция кадра", "帧线性插值", "매끄럽게 프레임 연결") \
  X(EDIT_FRAME_PIVOT, "Frame Pivot", "Точка вращения кадра", "帧枢轴", "프레임 중심점") \
  X(EDIT_FRAME_POSITION, "Frame Position", "Позиция кадра", "帧位置", "프레임 위치") \
  X(EDIT_FRAME_ROTATION, "Frame Rotation", "Поворот кадра", "帧旋转", "프레임 회전") \
  X(EDIT_FRAME_SCALE, "Frame Scale", "Масштаб кадра", "帧缩放", "프레임 비율") \
  X(EDIT_FRAME_SIZE, "Frame Size", "Размер кадра", "帧大小", "프레임 크기") \
  X(EDIT_FRAME_TINT, "Frame Tint", "Оттенок кадра", "帧色调", "프레임 색조") \
  X(EDIT_FRAME_VISIBILITY, "Frame Visibility", "Видимость кадра", "帧可见", "프레임 표시") \
  X(EDIT_GENERATE_ANIMATION_FROM_GRID, "Generate Animation from Grid", "Создать анимацию с сетки", "从网格转换为动画", "격자로 애니메이션 생성") \
  X(EDIT_INSERT_FRAME, "Insert Frame", "Вставить кадр", "插入帧", "프레임 삽입하기") \
  X(EDIT_LOOP, "Loop", "Цикл", "循环", "반복") \
  X(EDIT_MERGE_ANIMATIONS, "Merge Animations", "Соединить анимации", "合并多个动画", "애니메이션 병합") \
  X(EDIT_MERGE_ANM2, "Merge Anm2", "Соединить Anm2", "合并多个Anm2", "Anm2 병합") \
  X(EDIT_MOVE_ANIMATIONS, "Move Animation(s)", "Переместить анимации", "移动动画", "애니메이션 이동") \
  X(EDIT_MOVE_FRAMES, "Move Frame(s)", "Перемесить кадры", "移动多个/单个帧", "프레임 이동") \
  X(EDIT_MOVE_LAYER_ANIMATION, "Move Layer Animation", "Переместить анимацию слоя", "移动动画层", "레이어 애니메이션 이동") \
  X(EDIT_PASTE_ANIMATIONS, "Paste Animation(s)", "Вставить анимации", "粘贴动画", "애니메이션 붙여넣기") \
  X(EDIT_PASTE_EVENTS, "Paste Event(s)", "Вставить события", "粘贴事件", "이벤트 붙여넣기") \
  X(EDIT_PASTE_FRAMES, "Paste Frame(s)", "Вставить кадры", "粘贴帧", "프레임 붙여넣기") \
  X(EDIT_PASTE_LAYERS, "Paste Layer(s)", "Вставить слои", "粘贴动画层", "레이어 붙여넣기") \
  X(EDIT_PASTE_NULLS, "Paste Null(s)", "Вставить нули", "粘贴Null", "Null 붙여넣기") \
  X(EDIT_PASTE_SPRITESHEETS, "Paste Spritesheet(s)", "Вставить спрайт-листы", "粘贴图集", "스프라이트 시트 붙여넣기") \
  X(EDIT_RELOAD_SPRITESHEETS, "Reload Spritesheet(s)", "Перезагрузить спрайт-листы", "重新加载图集", "스프라이트 시트 다시 불러오기") \
  X(EDIT_REMOVE_ANIMATIONS, "Remove Animation(s)", "Удалить анимации", "删除动画层", "애니메이션 제거") \
  X(EDIT_REMOVE_ITEMS, "Remove Item(s)", "Удалить предметы", "删除物品", "항목 제거") \
  X(EDIT_REMOVE_UNUSED_EVENTS, "Remove Unused Events", "Удалить неизпользуемые события", "删除未使用的事件", "미사용 이벤트 제거") \
  X(EDIT_REMOVE_UNUSED_LAYERS, "Remove Unused Layers", "Удалить неизпользуемые слои", "删除未使用的动画层", "미사용 레이어 제거") \
  X(EDIT_REMOVE_UNUSED_NULLS, "Remove Unused Nulls", "Удалить неизпользуемые нули", "删除未使用的Null", "미사용 Null 제거") \
  X(EDIT_REMOVE_UNUSED_SOUNDS, "Remove Unused Sounds", "Удалить неизпользуемые звуки", "删除未使用的声音", "미사용 사운드 제거") \
  X(EDIT_REMOVE_UNUSED_SPRITESHEETS, "Remove Unused Spritesheets", "Удалить неизпользуемые спрайт-листы", "删除未使用的图集", "미사용 스프라이트 시트 제거") \
  X(EDIT_RENAME_EVENT, "Rename Event", "Переименовать событие", "重命名事件", "이벤트 이름 바꾸기") \
  X(EDIT_REPLACE_SPRITESHEET, "Replace Spritesheet", "Заменить спрайт-лист", "替换图集", "스프라이트 시트 교체") \
  X(EDIT_SET_LAYER_PROPERTIES, "Set Layer Properties", "Установить свойства слоя", "更改动画层属性", "레이어 속성 설정") \
  X(EDIT_SET_NULL_PROPERTIES, "Set Null Properties", "Установить свойства нуля", "更改Null属性", "Null 속성 설정") \
  X(EDIT_SPLIT_FRAME, "Split Frame", "Разделить кадр", "拆分帧", "프레임 분할") \
  X(EDIT_SHORTEN_FRAME, "Shorten Frame", "Укоротить кадр", "缩短帧时长", "프레임 단축") \
  X(EDIT_TOGGLE_ITEM_VISIBILITY, "Toggle Item Visibility", "Переключить видимость предмета", "物品可见", "항목 표시/숨기기") \
  X(EDIT_TOGGLE_NULL_RECT, "Toggle Null Rectangle", "Переключить прямоугольник нуля", "Null框可见", "Null 사각형 표시/숨기기") \
  X(EDIT_TRIGGER_AT_FRAME, "Trigger At Frame", "Активировать на кадре", "在指定帧触发", "트리거 프레임") \
  X(EDIT_TRIGGER_EVENT, "Trigger Event", "Событие триггера", "触发事件", "트리거 이벤트") \
  X(EDIT_TRIGGER_SOUND, "Trigger Sound", "Звук триггера", "触发声音", "트리거 사운드") \
  X(EDIT_TRIGGER_VISIBILITY, "Trigger Visibility", "Видимость триггера", "触发可见", "트리거 표시") \
  X(FORMAT_AT_FRAME, "At Frame: {0}", "На кадре: {0}", "第{0}帧", "트리거될 프레임: {0}") \
  X(FORMAT_COLOR_OFFSET, "Color Offset: {0}, {1}, {2}", "Смещение цвета: {0} {1} {2}", "颜色偏移: {0} {1} {2}", "색상 오프셋: {0} {1} {2}") \
  X(FORMAT_CROP, "Crop: ({0}, {1})", "Обрезка: ({0}, {1})", "裁剪: ({0}, {1})", "자르기: ({0}, {1})") \
  X(FORMAT_DURATION, "Duration: {0}", "Продолжительность: {0}", "时长: {0}", "유지 시간: {0}") \
  X(FORMAT_EVENT_LABEL, "Event: {0}", "Событие: {0}", "事件: {0}", "이벤트: {0}") \
  X(FORMAT_ID, "ID: {0}", "ID: {0}", "ID: {0}", "ID: {0}") \
  X(FORMAT_SPRITESHEET_ID, "Spritesheet ID: {0}", "", "图集 ID: {0}", "스프라이트 시트 ID: {0}") \
  X(FORMAT_INDEX, "Index: {0}", "Индекс: {0}", "下标: {0}", "인덱스: {0}") \
  X(FORMAT_INTERPOLATED, "Interpolated: {0}", "Интерполировано: {0}", "线性插值: {0}", "매끄럽게 연결: {0}") \
  X(FORMAT_LAYER, "#{0} {1} (Spritesheet: #{2})", "#{0} {1} (Спрайт-лист: #{2})", "#{0} {1} (图集: #{2})", "#{0} {1} (스프라이트 시트: #{2})") \
  X(FORMAT_LENGTH, "Length: {0}", "Длина: {0}", "长度: {0}", "길이: {0}") \
  X(FORMAT_LOOP, "Loop: {0}", "Цикл: {0}", "循环: {0}", "반복: {0}") \
  X(FORMAT_NOT_SAVED, "{0} [Not Saved]", "{0} [Не сохранено]", "{0} [未保存]", "{0} [저장되지 않음]") \
  X(FORMAT_NULL, "#{0} {1}", "#{0} {1}", "#{0} {1}", "#{0} {1}") \
  X(FORMAT_PIVOT, "Pivot: ({0}, {1})", "Точка вращения: ({0}, {1})", "枢轴: ({0}, {1})", "중심점: ({0}, {1})") \
  X(FORMAT_POSITION, "Position: ({0}, {1})", "Позиция: ({0}, {1})", "位置: ({0}, {1})", "위치: ({0}, {1})") \
  X(FORMAT_POSITION_SPACED, "Position: ({0:8}, {1:8})", "Позиция: ({0:8}, {1:8})", "位置: ({0:8}, {1:8})", "위치: ({0:8}, {1:8})") \
  X(FORMAT_ROTATION, "Rotation: {0}", "Поворот: {0}", "旋转: {0}", "회전: {0}") \
  X(FORMAT_SCALE, "Scale: ({0}, {1})", "Масштаб: ({0}, {1})", "缩放: ({0}, {1})", "비율: ({0}, {1})") \
  X(FORMAT_SIZE, "Size: ({0}, {1})", "Размер: ({0}, {1})", "大小: ({0}, {1})", "크기: ({0}, {1})") \
  X(FORMAT_SOUND_LABEL, "Sound: {0}", "Звук: {0}", "声音: {0}", "사운드: {0}") \
  X(FORMAT_SPRITESHEET, "#{0} {1}", "#{0} {1}", "#{0} {1}", "#{0} {1}") \
  X(FORMAT_TRANSFORM, "Transform: {0}", "Трансформация: {0}", "变换: {0}", "변환: {0}") \
  X(FORMAT_RECT, "Rect: {0}", "Прямоугольник: {0}", "矩形: {0}", "사각형: {0}") \
  X(FORMAT_FRAMES_COUNT, "Frames: {0}", "Кадры: {0}", "帧: {0}", "프레임: {0}") \
  X(FORMAT_TRIGGERS_COUNT, "Triggers: {0}", "Триггеры: {0}", "触发器: {0}", "트리거: {0}") \
  X(FORMAT_TEXTURE_SIZE, "Size: {0}x{1}", "Размер: {0}x{1}", "大小: {0}x{1}", "크기: {0}x{1}") \
  X(FORMAT_TINT, "Tint: {0}, {1}, {2}, {3}", "Оттенок: {0}, {1}, {2}, {3}", "色调: {0}, {1}, {2}, {3}", "색조: {0}, {1}, {2}, {3}") \
  X(FORMAT_TOOLTIP_SHORTCUT, "{0}\n(Shortcut: {1})", "{0}\n(Сочетание клавиш: {1})", "{0}\n(快捷键: {1})", "{0}\n(단축키: {1})") \
  X(FORMAT_VISIBLE, "Visible: {0}", "Видимо: {0}", "可见: {0}", "표시: {0}") \
  X(LABEL_ADJUST, "Adjust", "Отрегулировать", "调节", "조정") \
  X(LABEL_ALT_ICONS, "Alt Icons", "Альт-иконки", "替代图标", "대체 아이콘") \
  X(LABEL_ANIMATIONS_CHILD, "Animations", "", "动画", "애니메이션") \
  X(LABEL_ANIMATIONS_MERGE_POPUP, "Merge Animations", "Соединить анимации", "合并多个动画", "애니메이션 병합") \
  X(LABEL_ANIMATIONS_WINDOW, "Animations###Animations", "Анимации###Animations", "动画###Animations", "애니메이션###Animations") \
  X(LABEL_ANIMATION_LENGTH, "Animation Length", "Длина анимации", "动画时长", "애니메이션 길이") \
  X(LABEL_ANIMATION_PREVIEW_WINDOW, "Animation Preview###Animation Preview", "Предпросмотр анимации###Animation Preview", "动画预放###Animation Preview", "애니메이션 프리뷰###Animation Preview") \
  X(LABEL_APPEND_FRAMES, "Append Frames", "Добавить кадры к концу", "在后面添加帧", "뒷프레임에 추가") \
  X(LABEL_APPLICATION_NAME, "Anm2Ed", "Anm2Ed", "Anm2Ed", "Anm2Ed") \
  X(LABEL_APPLICATION_VERSION, "Version 2.1", "Версия 2.1", "2.1版本", "버전 2.1") \
  X(LABEL_AUTHOR, "Author", "Автор", "制作者", "작성자") \
  X(LABEL_AUTOSAVE, "Autosave", "Автосохранение", "自动保存", "자동저장") \
  X(LABEL_AXES, "Axes", "Оси", "坐标轴", "가로/세로 축") \
  X(LABEL_BACKGROUND_COLOR, "Background", "Фон", "背景", "배경색") \
  X(LABEL_BAKE, "Bake", "Запечь", "提前渲染", "베이크") \
  X(LABEL_SPLIT, "Split", "Разделить", "拆分", "분할") \
  X(LABEL_BORDER, "Border", "Границы", "边框", "경계선") \
  X(LABEL_CENTER_VIEW, "Center View", "Центрировать вид", "视角中心", "가운데서 보기") \
  X(LABEL_CLAMP, "Clamp", "Ограничить", "限制数值范围", "작업 영역 제한") \
  X(LABEL_CLEAR_LIST, "Clear List", "Стереть список", "清除列表", "기록 삭제") \
  X(LABEL_CLOSE, "Close", "Закрыть", "关闭", "닫기") \
  X(LABEL_CUSTOM_RANGE, "Custom Range", "Пользовательский диапазон", "自定义范围", "길이 맞춤설정") \
  X(LABEL_DELETE, "Delete", "Удалить", "删除", "삭제") \
  X(LABEL_DELETE_ANIMATIONS_AFTER, "Delete Animations After", "Удалить анимации после", "删除之后的动画", "기존 애니메이션 삭제") \
  X(LABEL_DISPLAY, "Display", "Отображение", "显示", "디스플레이") \
  X(LABEL_DIVIDE, "Divide", "Разделить", "分开", "나누기") \
  X(LABEL_DOCUMENTS_MERGE_INTO_CURRENT, "Merge into Current Document", "Соединить в текущий документ", "合并到当前的文件里", "현재 파일에 병합") \
  X(LABEL_DOCUMENTS_OPEN_MANY, "Open Many Documents", "Открыть несколько документов", "打开许多文件", "여러 파일 열기") \
  X(LABEL_DOCUMENTS_OPEN_NEW, "Open New Document", "Открыть новый документ", "打开新文件", "새 파일로 열기") \
  X(LABEL_DOCUMENT_CLOSE, "Close Document", "Закрыть документ", "关闭文件", "파일 닫기") \
  X(LABEL_DOCUMENT_MODIFIED_PROMPT, "The document \"{0}\" has been modified.\nDo you want to save it?", "Документ \"{0}\" был изменен. \nХотите сохранить его?", "此文件\"{0}\"已被更改.\n要保存吗？", "\"{0}\" 파일이 수정되었습니다.\n저장하시겠습니까?") \
  X(LABEL_END, "End", "Конец", "结尾", "끝") \
  X(LABEL_EVENT, "Event", "Событие", "事件", "이벤트") \
  X(LABEL_EVENTS_WINDOW, "Events###Events", "События###Events", "事件###Events", "이벤트###Events") \
  X(LABEL_EXISTING, "Existing", "Существующий", "已存在的", "기존") \
  X(LABEL_EXIT, "Exit", "Выход", "退出", "종료") \
  X(LABEL_EXPLORE_XML_LOCATION, "Explore XML Location", "Открыть местоположение XML", "探索XML位置", "XML 경로로 탐색기 열기") \
  X(LABEL_FFMPEG_PATH, "FFmpeg Path", "Путь к FFmpeg", "FFmpeg路径", "FFmpeg 경로") \
  X(LABEL_FILE_MENU, "File", "Файл", "文件", "파일") \
  X(LABEL_FIT, "Fit", "Подогнать по размеру", "匹配", "맞추기") \
  X(LABEL_FIT_ANIMATION_LENGTH, "Fit Animation Length", "Подогнать к длине анимации", "匹配动画时长", "애니메이션 길이 맞추기") \
  X(LABEL_FLIP_X, "Flip X", "Отразить по X", "X轴翻转", "수평 뒤집기") \
  X(LABEL_FLIP_Y, "Flip Y", "Отразить по Y", "Y轴翻转", "수직 뒤집기") \
  X(LABEL_FORMAT, "Format", "Формат", "格式", "파일 포맷") \
  X(LABEL_FPS, "FPS", "FPS", "FPS", "FPS") \
  X(LABEL_FRAME_PROPERTIES_WINDOW, "Frame Properties###Frame Properties", "Свойства кадра###Frame Properties", "帧属性###Frame Properties", "프레임 속성###Frame Properties") \
  X(LABEL_GENERATE, "Generate", "Сгенерировать", "生成", "생성") \
  X(LABEL_GENERATE_COLUMNS, "Columns", "Колонны", "列", "열") \
  X(LABEL_GENERATE_COUNT, "Count", "Кол-во", "数量", "프레임 수") \
  X(LABEL_GENERATE_FRAME_SIZE, "Frame Size", "Размер кадра", "单帧大小", "프레임 크기") \
  X(LABEL_GENERATE_ROWS, "Rows", "Ряды", "行", "행") \
  X(LABEL_GENERATE_START_POSITION, "Start Position", "Начальная позиция", "起始位置", "시작 위치") \
  X(LABEL_GLOBAL, "Global", "Глобальный", "全局", "전역 추가") \
  X(LABEL_HELP_MENU, "Help", "Помощь", "帮助", "도움말") \
  X(LABEL_IGNORE_FRAMES, "Ignore Frames", "Игнорировать кадры", "忽略帧", "프레임 무시") \
  X(LABEL_INPUT, "Input", "Ввод", "输入", "입력") \
  X(LABEL_INSERT, "Insert", "Вставить", "插入", "삽입") \
  X(LABEL_INTERVAL, "Interval", "Промежуток", "区间", "간격") \
  X(LABEL_KEYBOARD, "Keyboard", "Клавиатура", "键盘", "키보드") \
  X(LABEL_LANGUAGE, "Language", "Язык", "语言", "언어") \
  X(LABEL_LAYER, "Layer", "Слой", "动画层", "레이어") \
  X(LABEL_LAYERS_CHILD, "Layers List", "", "动画层列表", "레이어 목록") \
  X(LABEL_LAYERS_WINDOW, "Layers###Layers", "Слои###Layers", "动画层###Layers", "레이어###Layers") \
  X(LABEL_LENGTH, "Length", "", "长度", "길이") \
  X(LABEL_LOCAL, "Local", "Локальный", "局部", "지역 추가") \
  X(LABEL_LOCALE, "Locale", "Локаль", "局部", "추가할 위치") \
  X(LABEL_LOCALIZATION, "Localization", "Локализация", "本地化", "현지화") \
  X(LABEL_LOOP, "Loop", "Цикл", "循环", "반복") \
  X(LABEL_MANAGER_ANM2_DRAG_DROP, "Anm2 Drag Drop", "Anm2 Drag Drop", "Anm2 拖放", "Anm2 드래그 앤 드롭") \
  X(LABEL_MANAGER_LAYER_PROPERTIES, "Layer Properties", "Свойства слоя", "动画层属性", "레이어 속성") \
  X(LABEL_MANAGER_NULL_PROPERTIES, "Null Properties", "Свойства нуля", "Null属性", "Null 속성") \
  X(LABEL_MANAGER_RENDERING_PROGRESS, "Rendering...", "Рендеринг...", "渲染中...", "렌더링 중...") \
  X(LABEL_MERGE, "Merge", "Соединить", "合并", "병합") \
  X(LABEL_MOVE_TOOL_SNAP, "Move Tool: Snap to Mouse", "Инструмент перемещения: Привязка к мыши", "移动工具: 吸附到鼠标指针", "이동 도구: 마우스에 맞추기") \
  X(LABEL_MULTIPLY, "Multiply", "Умножить", "乘", "곱하기") \
  X(LABEL_NEW, "New", "Новый", "新", "새로") \
  X(LABEL_NULL, "Null", "Нуль", "Null", "Null") \
  X(LABEL_NULLS_WINDOW, "Nulls###Nulls", "Нули###Nulls", "Nulls###Nulls", "Null###Nulls") \
  X(LABEL_ONIONSKIN_WINDOW, "Onionskin###Onionskin", "Оньонскин###Onionskin", "洋葱皮预览###Onionskin", "전후 비교###Onionskin") \
  X(LABEL_OPEN_RECENT, "Open Recent", "Открыть недавние", "打开最近", "최근 파일 열기") \
  X(LABEL_OPTIONS, "Options", "Настройки", "选项", "설정") \
  X(LABEL_OUTPUT_DIRECTORY, "Directory", "Директория", "目录", "디렉터리") \
  X(LABEL_OUTPUT_PATH, "Path", "Путь", "路径", "경로") \
  X(LABEL_OVERLAY, "Overlay", "Наложение", "动画叠加", "오버레이") \
  X(LABEL_OVERWRITE_CONFIRMATION, "Are you sure? This will overwrite the existing file.", "Вы уверены? Это перезапишет существующий файл.", "你确定吗? 这样会覆盖原本的文件.", "저장하시겠습니까? 기존 파일을 덮어쓰게 됩니다.") \
  X(LABEL_OVERWRITE_WARNING, "Overwrite Warning", "Предупреждение о перезаписи", "覆盖提醒", "덮어쓰기 경고") \
  X(LABEL_PAUSE, "Pause", "Пауза", "暂停", "일시정지") \
  X(LABEL_PIVOTS, "Pivots", "Точки вращения", "枢轴", "중심점") \
  X(LABEL_PLAY, "Play", "Возпроизвести", "播放", "재생") \
  X(LABEL_PLAYBACK_ALWAYS_LOOP, "Always Loop", "Всегда воспроизводить циклично", "持续循环", "항상 반복") \
  X(LABEL_PLAYBACK_MENU, "Playback", "Возпроизведение", "回放", "재생") \
  X(LABEL_PREPEND_FRAMES, "Prepend Frames", "Добавить кадры", "在前面插入帧", "뒷프레임에 추가") \
  X(LABEL_RAW, "Raw", "Необработанный", "原始", "Raw만") \
  X(LABEL_RECT, "Rect", "Прямоугольник", "矩形", "사각형") \
  X(LABEL_REMOVE_UNUSED_LAYERS, "Remove Unused", "", "删除未使用", "미사용 레이어 제거") \
  X(LABEL_REMOVE_UNUSED_NULLS, "Remove Unused", "", "删除未使用", "미사용 Null 제거") \
  X(LABEL_REMOVE_UNUSED_SPRITESHEETS, "Remove Unused", "", "删除未使用", "미사용 스프라이트 시트 제거") \
  X(LABEL_RENDER, "Render", "Рендер", "渲染", "렌더링") \
  X(LABEL_REPEAT_DELAY, "Repeat Delay (seconds)", "Задержка повторения (секунды)", "键盘按键重复延迟 (秒)", "반복 입력 지연 시간 (초)") \
  X(LABEL_REPEAT_RATE, "Repeat Rate (seconds)", "Скорость повторения (секунды)", "键盘按键重复速率 (秒)", "반복 입력 속도 (초)") \
  X(LABEL_REPLACE_FRAMES, "Replace Frames", "Заменить кадры", "替换帧", "프레임 교체") \
  X(LABEL_RESTORE_AUTOSAVES_PROMPT, "Autosaved documents detected. Would you like to restore them?", "Автосохраненные документы найдены. Хотите их восстановить?", "检测到自动保存的文件. 需要恢复吗?", "자동저장된 파일이 감지되었습니다. 복원하시겠습니까?") \
  X(LABEL_ROOT_TRANSFORM, "Root Transform", "Трансформация корня", "根变换", "Root 변환") \
  X(LABEL_ROUND_ROTATION, "Round Rotation", "Округленный поворот", "旋转取整", "회전율 반올림") \
  X(LABEL_ROUND_SCALE, "Round Scale", "Округленный масштаб", "缩放取整", "비율 반올림") \
  X(LABEL_SAVE_AS, "Save As", "Сохранить как", "保存为", "다른 이름으로 저장") \
  X(LABEL_SETTINGS_MENU, "Settings", "Настройки", "设置", "설정") \
  X(LABEL_SET_TO_RECOMMENDED, "Set to Recommended", "Установить на рекомендованные", "设置为推荐值", "권장값으로 설정") \
  X(LABEL_SHORTCUTS_TAB, "Shortcuts", "Сочетания клавиш", "快捷键", "단축키") \
  X(LABEL_SHORTCUT_COLUMN, "Shortcut", "Сочетание клавиш", "快捷方式", "단축") \
  X(LABEL_SNAP, "Snap", "Привязка", "吸附", "맞추기") \
  X(LABEL_SNAPSHOTS, "Snapshots", "Снапшоты", "快照", "스냅숏") \
  X(LABEL_SOUND, "Sound", "Звук", "声音", "사운드") \
  X(LABEL_SOUNDS_WINDOW, "Sounds###Sounds", "Звук###Sounds", "声音###Sounds", "사운드###Sounds") \
  X(LABEL_SOURCE, "Source", "Източник", "动画层来源", "소스") \
  X(LABEL_SPRITESHEET, "Spritesheet", "Спрайт-лист", "图集", "스프라이트 시트") \
  X(LABEL_SPRITESHEETS_WINDOW, "Spritesheets###Spritesheets", "Спрайт-листы###Spritesheets", "图集###Spritesheets", "스프라이트 시트###Spritesheets") \
  X(LABEL_SPRITESHEET_EDITOR_WINDOW, "Spritesheet Editor###Spritesheet Editor", "Редактор спрайт-листов###Spritesheet Editor", "图集编辑器###Spritesheet Editor", "스프라이트 편집기###Spritesheet Editor") \
  X(LABEL_STACK_SIZE, "Stack Size", "Размер стека", "栈内存大小", "스택 크기") \
  X(LABEL_START, "Start", "Старт", "开始", "시작") \
  X(LABEL_SUBTRACT, "Subtract", "Вычетание", "减去", "빼기") \
  X(LABEL_TASKBAR_ABOUT, "About", "О", "关于", "정보") \
  X(LABEL_TASKBAR_CONFIGURE, "Configure", "Конфигурация", "配置", "구성 설정") \
  X(LABEL_TASKBAR_GENERATE_ANIMATION_FROM_GRID, "Generate Animation from Grid", "Создать анимацию с сетки", "从网格转换为动画", "격자로 애니메이션 생성") \
  X(LABEL_TASKBAR_OVERWRITE_FILE, "Overwrite File", "Перезапись файла", "覆盖文件", "파일 덮어쓰기") \
  X(LABEL_TASKBAR_RENDER_ANIMATION, "Render Animation", "Рендер анимации", "生成动画", "애니메이션 렌더링") \
  X(LABEL_THEME, "Theme", "Тема", "主题", "테마") \
  X(LABEL_THEME_CLASSIC, "ImGui Classic", "Классическая ImGui", "经典ImGui", "ImGui 클래식") \
  X(LABEL_THEME_DARK, "Dark", "Темная", "深色", "어둡게") \
  X(LABEL_THEME_LIGHT, "Light", "Светлая", "亮色", "밝게") \
  X(LABEL_TIMELINE_BAKE_POPUP, "Bake", "Запечь", "提前渲染", "베이크") \
  X(LABEL_TIMELINE_PROPERTIES_POPUP, "Item Properties", "Свойства предмета", "物品属性", "항목 속성") \
  X(LABEL_TIMELINE_WINDOW, "Timeline###Timeline", "Таймлайн###Timeline", "时间轴###Timeline", "타임라인###Timeline") \
  X(LABEL_TIME_MINUTES, "Time (minutes)", "Время (минуты)", "时间 (分钟)", "시간 (분)") \
  X(LABEL_TOOL, "Tool", "Инструмент", "工具", "도구") \
  X(LABEL_TOOLS_COLOR_EDIT_POPUP, "##Color Edit", "##Color Edit", "##Color Edit", "##Color Edit") \
  X(LABEL_TOOLS_WINDOW, "Tools###Tools", "Инструменты###Tools", "工具###Tools", "도구###Tools") \
  X(LABEL_TO_ANIMATION_RANGE, "To Animation Range", "К диапазону анимации", "到动画范围", "애니메이션 범위에 맞춤") \
  X(LABEL_TO_SELECTED_FRAMES, "To Selected Frames", "К выбранным кадрам", "跳到所选的帧", "선택한 프레임에 맞춤") \
  X(LABEL_TRANSPARENT, "Transparent", "Прозрачный", "透明", "투명도") \
  X(LABEL_TYPE, "Type", "Тип", "类型", "유형") \
  X(LABEL_UI_SCALE, "UI Scale", "Размер UI", "界面缩放", "UI 비율") \
  X(LABEL_USE_DEFAULT_SETTINGS, "Use Default Settings", "Изпользовать настройки по умолчанию", "使用默认设置", "기본값으로 사용") \
  X(LABEL_VALUE_COLUMN, "Value", "Значение", "数值", "값") \
  X(LABEL_VSYNC, "Vsync", "Вертикальная синхронизация (V-sync)", "垂直同步", "수직 동기화") \
  X(LABEL_WELCOME_DESCRIPTION, "Select a recent file or open a new or existing document. You can also drag and drop files into the window to open them.", "Выберите недавний файл или откройте новый или существующий документ. Вы также можете перетащить файлы в это окно, чтобы их открыть.", "选择最近的文件, 或打开新的/已有的文档. 你也可以将文件拖拽到窗口中以打开它们.", "최근 파일을 선택하거나 새 파일 또는 기존 파일을 엽니다. 파일을 여기로 드래그하여 열 수 있습니다.") \
  X(LABEL_WELCOME_RESTORE_POPUP, "Restore", "Восстановиь", "恢复", "복원") \
  X(LABEL_WELCOME_WINDOW, "Welcome###Welcome", "Добро пожаловать###Welcome", "欢迎###Welcome", "환영합니다###Welcome") \
  X(LABEL_WINDOW_MENU, "Window", "Окно", "窗口", "창") \
  X(LABEL_WIZARD_MENU, "Wizard", "Помощник", "向导", "보조 도구") \
  X(LABEL_ZOOM, "Zoom", "Масштаб", "视角缩放", "확대") \
  X(LABEL_ZOOM_STEP, "Step", "Шаг", "步长", "단계") \
  X(SHORTCUT_STRING_ADD, "Add", "Добавить", "添加", "추가") \
  X(SHORTCUT_STRING_CENTER_VIEW, "Center View", "Центрировать вид", "视角居中", "가운데서 보기") \
  X(SHORTCUT_STRING_CLOSE, "Close", "Закрыть", "关闭", "닫기") \
  X(SHORTCUT_STRING_COLOR, "Color", "Цвет", "颜色", "색상") \
  X(SHORTCUT_STRING_COLOR_PICKER, "Color Picker", "Выбор цвета", "取色器", "색상 선택기") \
  X(SHORTCUT_STRING_COPY, "Copy", "Копировать", "复制", "복사") \
  X(SHORTCUT_STRING_CROP, "Crop", "Обрезать", "裁剪", "자르기") \
  X(SHORTCUT_STRING_CUT, "Cut", "Вырезать", "剪切", "잘라내기") \
  X(SHORTCUT_STRING_DEFAULT, "Default", "По умолчанию", "默认", "기본값") \
  X(SHORTCUT_STRING_DRAW, "Draw", "Рисовать", "绘画", "그리기") \
  X(SHORTCUT_STRING_DUPLICATE, "Duplicate", "Дублировать", "拷贝", "복제") \
  X(SHORTCUT_STRING_ERASE, "Erase", "Стереть", "擦除", "지우기") \
  X(SHORTCUT_STRING_EXIT, "Exit", "Выйти", "退出", "종료") \
  X(SHORTCUT_STRING_EXTEND_FRAME, "Extend Frame", "Удлиннить кадр", "延长帧", "프레임 확장") \
  X(SHORTCUT_STRING_FIT, "Fit", "Подогнать", "匹配", "맞추기") \
  X(SHORTCUT_STRING_INSERT_FRAME, "Insert Frame", "Вставить кадр", "插入帧", "프레임 삽입") \
  X(SHORTCUT_STRING_MERGE, "Merge", "Соединить", "合并", "병합") \
  X(SHORTCUT_STRING_MOVE, "Move", "Передвинуть", "移动", "이동") \
  X(SHORTCUT_STRING_NEW, "New", "Новый", "新", "새 파일") \
  X(SHORTCUT_STRING_NEXT_FRAME, "Next Frame", "Следующий кадр", "下一帧", "다음 프레임") \
  X(SHORTCUT_STRING_ONIONSKIN, "Onionskin", "Оньонскин", "洋葱皮预览", "전후 비교") \
  X(SHORTCUT_STRING_OPEN, "Open", "Открыть", "打开", "열기") \
  X(SHORTCUT_STRING_PAN, "Pan", "Панорамирование", "平移", "시점 이동") \
  X(SHORTCUT_STRING_PASTE, "Paste", "Вставить", "粘贴", "붙여넣기") \
  X(SHORTCUT_STRING_PLAYHEAD_BACK, "Playhead Back", "Назад воспроизводящей головкой", "播放指针往后", "플레이헤드 뒤로") \
  X(SHORTCUT_STRING_PLAYHEAD_FORWARD, "Playhead Forward", "Вперед воспроизводящей головкой", "播放指针往前", "플레이헤드 앞으로") \
  X(SHORTCUT_STRING_PLAY_PAUSE, "Play/Pause", "Возпроизвести/Пауза", "播放/暂停", "재생/일시정지") \
  X(SHORTCUT_STRING_PREVIOUS_FRAME, "Previous Frame", "Предыдущий кадр", "前一帧", "이전 프레임") \
  X(SHORTCUT_STRING_REDO, "Redo", "Повторить", "重做", "다시 실행") \
  X(SHORTCUT_STRING_REMOVE, "Remove", "Удалить", "去除", "제거") \
  X(SHORTCUT_STRING_ROTATE, "Rotate", "Поворачивать", "旋转", "회전") \
  X(SHORTCUT_STRING_SAVE, "Save", "Сохранить", "保存", "저장") \
  X(SHORTCUT_STRING_SAVE_AS, "Save As", "Сохранить как", "保存为", "다른 이름으로 저장") \
  X(SHORTCUT_STRING_SCALE, "Scale", "Изменить масштаб", "缩放", "비율") \
  X(SHORTCUT_STRING_SPLIT, "Split", "", "", "") \
  X(SHORTCUT_STRING_BAKE, "Bake", "Запечь", "提前渲染", "베이크") \
  X(SHORTCUT_STRING_SHORTEN_FRAME, "Shorten Frame", "Укоротить кадр", "缩短帧时长", "프레임 단축") \
  X(SHORTCUT_STRING_UNDO, "Undo", "Отменить", "撤销", "실행 취소") \
  X(SHORTCUT_STRING_ZOOM_IN, "Zoom In", "Увеличить", "视图放大", "확대") \
  X(SHORTCUT_STRING_ZOOM_OUT, "Zoom Out", "Уменьшить", "视图缩小", "축소") \
  X(SNAPSHOT_RENAME_ANIMATION, "Rename Animation", "Переименовать анимацию", "重命名动画", "애니메이션 이름 바꾸기") \
  X(TEXT_NEW_ANIMATION, "New Animation", "Новая анимация", "新动画", "새 애니메이션") \
  X(TEXT_NEW_EVENT, "New Event", "Новое событие", "新事件", "새 이벤트") \
  X(TEXT_RECORDING_PROGRESS, "Once recording is complete, rendering may take some time.\nPlease be patient...", "Когда запись завершена, рендеринг может занять некоторое время.\nПожалуйста потерпите...", "录制完成时，渲染可能会花一些时间.\n请耐心等待...", "녹화가 완료되면 렌더링에 시간이 걸릴 수 있습니다.\n잠시만 기다려 주세요...") \
  X(TEXT_OPEN_DIRECTORY, "Double-click to open directory in file explorer.", "Дважды нажмите, чтобы открыть директорию в проводнике.", "双击以在文件管理器中打开目录。", "더블 클릭하여 파일 탐색기로 디렉터리를 엽니다.") \
  X(TOAST_AUTOSAVE_FAILED, "Could not autosave document to: {0} ({1})", "Не получилось автосохранить документ в: {0} ({1})", "自动保存到 {0} ({1}) 失败", "{0}에 파일을 자동저장할 수 없습니다. ({1})") \
  X(TOAST_AUTOSAVING, "Autosaving...", "Автосохранение...", "自动保存中...", "자동저장 중...") \
  X(TOAST_DESERIALIZE_ANIMATIONS_FAILED, "Failed to deserialize animation(s): {0}", "Не удалось десериализировать анимации: {0}", "反序列化动画失败: {0}", "애니메이션 역직렬화 실패: {0}") \
  X(TOAST_DESERIALIZE_EVENTS_FAILED, "Failed to deserialize event(s): {0}", "Не удалось десериализировать события: {0}", "反序列化事件失败: {0}", "이벤트 역직렬화 실패: {0}") \
  X(TOAST_DESERIALIZE_FRAMES_FAILED, "Failed to deserialize frames:", "Не удалось десериализировать кадры: {0}", "反序列化帧失败: {0}", "프레임 역직렬화 실패: {0}") \
  X(TOAST_DESERIALIZE_FRAMES_NO_SELECTION, "Failed to deserialize frames: no selection.", "Не удалось десериализировать кадры: ничего не выбрано.", "到所选帧", "프레임 역직렬화 실패. 선택한 것이 없습니다.") \
  X(TOAST_DESERIALIZE_LAYERS_FAILED, "Failed to deserialize layer(s):", "Не удалось десериализировать слои: {0}", "反序列化动画层失败: {0}", "레이어 역직렬화 실패: {0}") \
  X(TOAST_DESERIALIZE_NULLS_FAILED, "Failed to deserialize null(s): {0}", "Не удалось десериализировать нули: {0}", "反序列化Null失败: {0}", "Null 역직렬화 실패: {0}") \
  X(TOAST_DESERIALIZE_SPRITESHEETS_FAILED, "Failed to deserialize spritesheet(s): {0}", "Не удалось десериализировать спрайт-листы: {0}", "反序列化图集失败: {0}", "스프라이트 시트 역직렬화 실패: {0}") \
  X(TOAST_EXPORT_RENDERED_ANIMATION, "Exported rendered animation to: {0}", "Рендерированные анимации экспортированы в: {0}", "渲染动画导出至: {0}", "{0}에 렌더링 된 애니메이션을 내보내기 했습니다.") \
  X(TOAST_EXPORT_RENDERED_ANIMATION_FAILED, "Could not output rendered animation: {0}", "Не удалось вывести рендерированную анимацию: {0}", "无法输出渲染动画: {0}", "{0}에 렌더링 된 애니메이션을 내보내기 할 수 없습니다.") \
  X(TOAST_EXPORT_RENDERED_FRAMES, "Exported rendered frames to: {0}", "Рендерированные кадры экспортированы в: {0}", "渲染帧导出至: {0}", "{0}에 렌더링 된 프레임을 내보내기 했습니다.") \
  X(TOAST_EXPORT_RENDERED_FRAMES_FAILED, "Could not export frames to: {0}", "Не удалось экспортировать кадры в: {0}", "无法导出帧至: {0}", "프레임을 내보내기 할 수 없습니다. {0}") \
  X(TOAST_EXPORT_SPRITESHEET, "Exported spritesheet to: {0}", "Спрайт-лист экспортирован в: {0}", "导出图集至: {0}", "{0}에 스프라이트 시트를 내보내기 했습니다.") \
  X(TOAST_EXPORT_SPRITESHEET_FAILED, "Could not export spritesheet to: {0}", "Не удалось экспортировать спрайт-лист в: {0}", "无法导出图集至: {0}", "{0}에 스프라이트 시트를 내보내기 할 수 없습니다.") \
  X(TOAST_ADD_SPRITESHEET_FAILED, "Failed to add spritesheet! Open a document first.", "Не удалось добавить спрайт-лист! Сначала откройте документ.", "无法添加图集！请先打开文档。", "스프라이트 시트를 추가할 수 없습니다! 먼저 문서를 여세요.") \
  X(TOAST_ADD_SOUND_FAILED, "Failed to add sound! Open a document first.", "Не удалось добавить звук! Сначала откройте документ.", "无法添加声音！请先打开文档。", "사운드를 추가할 수 없습니다! 먼저 문서를 여세요.") \
  X(TOAST_INVALID_FFMPEG_PATH, "FFmpeg path must point to a valid executable file.", "Путь к FFmpeg должен указывать к валидному файлу .exe.", "FFmpeg路径必须指向一个正确的可执行文件.", "FFmpeg 경로는 유효한 실행 파일을 지정해야 합니다.") \
  X(TOAST_NOT_SUPPORTED, "Operation not supported.", "Операция не поддерживается.", "不支持此操作.", "해당 작업은 지원되지 않습니다.") \
  X(TOAST_OPEN_DOCUMENT, "Opened document: {0}", "Открыт документ: {0}", "已打开文件: {0}", "{0} 파일 열기") \
  X(TOAST_OPEN_DOCUMENT_FAILED, "Failed to open document: {0} ({1})", "Не удалось открыть документ: {0} ({1})", "打开文件失败: {0} ({1})", "{0} 파일을 여는데 실패했습니다. {1}") \
  X(TOAST_PNG_DIRECTORY_ACCESS_ERROR, "Could not access directory: {0} ({1})", "Не удалось получить доступ к директории: {0} ({1})", "无法访问目录: {0} ({1})", "{0} 디렉터리에 접근할 수 없습니다. {1}") \
  X(TOAST_PNG_DIRECTORY_CREATE_ERROR, "Could not create directory: {0} ({1})", "Не удалось создать директорию: {0} ({1})", "无法创造目录: {0} ({1})", "{0} 디렉터리를 생성할 수 없습니다. {1}") \
  X(TOAST_PNG_DIRECTORY_NOT_DIRECTORY, "PNG output path must be a directory: {0}", "Выходной путь для PNG должен быть директорией: {0}", "PNG输出路径必须是一个目录: {0}", "PNG를 출력할 경로는 디렉터리여야 합니다. {0}") \
  X(TOAST_PNG_DIRECTORY_NOT_SET, "PNG output directory must be set.", "Директория для вывода PNG должна быть установлена.", "必须设置PNG输出目录.", "PNG를 출력할 경로를 설정해야 합니다.") \
  X(TOAST_REDO, "Redo: {0}", "Повтор: {0}", "重做: {0}", "다시 실행: {0}") \
  X(TOAST_RELOAD_SPRITESHEET, "Reloaded spritesheet #{0}: {1}", "Спрайт-лист #{0} перезагружен: {1}", "重新加载了图集 #{0}: {1}", "{0}번 스프라이트 시트 다시 불러옴: {1}") \
  X(TOAST_REMOVE_SPRITESHEET, "Removed spritesheet #{0}: {1}", "Спрайт-лист #{0} удален: {1}", "去除了图集 #{0}: {1}", "{0}번 스프라이트 시트 제거됨: {1}") \
  X(TOAST_REPLACE_SPRITESHEET, "Replaced spritesheet #{0}: {1}", "Спрайт-лист #{0} заменен: {1}", "替换了图集 #{0}: {1}", "{0}번 스프라이트 시트 교체됨: {1}") \
  X(TOAST_SAVE_DOCUMENT, "Saved document to: {0}", "Документ сохранен в: {0}", "保存文件至: {0}", "{0}에 파일을 저장했습니다.") \
  X(TOAST_SAVE_DOCUMENT_FAILED, "Could not save document to: {0} ({1})", "Не удалось сохранить документ в: {0} ({1})", "无法保存文件至: {0} ({1})", "{0}에 파일을 저장할 수 없습니다.") \
  X(TOAST_SAVE_SPRITESHEET, "Saved spritesheet #{0}: {1}", "Спрайт-лист #{0} сохранен: {1}", "已保存图集 #{0}: {1}", "{1}에 {0}번 스프라이트 시트를 저장했습니다.") \
  X(TOAST_SAVE_SPRITESHEET_FAILED, "Unable to save spritesheet #{0}: {1}", "Не удалось сохранить спрайт-лист #{0}: {1}", "无法保存图集 #{0}: {1}", "{0}번 스프라이트 시트를 저장할 수 없습니다. {1}") \
  X(TOAST_SOUNDS_DESERIALIZE_ERROR, "Failed to deserialize sound(s): {0}", "Не удалось десериализировать звуки: {0}", "反序列化声音失败: {0}", "사운드 역직렬화 실패: {0}") \
  X(TOAST_SOUNDS_PASTE, "Paste Sound(s)", "Вставить звуки", "粘贴声音", "사운드 붙여넣기") \
  X(TOAST_SOUND_INITIALIZED, "Initialized sound #{0}: {1}", "Звук #{0} инициализирован: {1}", "已初始化声音 #{0}: {1}", "{0}번 사운드 초기화됨: {1}") \
  X(TOAST_SOUND_INITIALIZE_FAILED, "Failed to initialize sound: {0}", "Не удалось инициализировать звуки: {0}", "初始化声音失败: {0}", "사운드 초기화 실패: {0}") \
  X(TOAST_SPRITESHEET_EMPTY, "Spritesheet export failed: captured frames are empty.", "Не удалось экспортировать спрайт-лист: захваченные кадры пусты.", "导出图集失败: 未捕获任何帧.", "스프라이트 내보내기 실패: 캡처된 프레임이 비어있습니다.") \
  X(TOAST_SPRITESHEET_INITIALIZED, "Initialized spritesheet #{0}: {1}", "Спрайт-лист #{0} инициализирован: {1}", "已初始化图集 #{0}: {1}", "{0}번 스프라이트 시트 초기화됨: {1}") \
  X(TOAST_SPRITESHEET_INIT_FAILED, "Failed to initialize spritesheet: {0}", "Не удалось инициализировать спрайт-лист: {0}", "初始化图集失败: {0}", "스프라이트 시트 초기화 실패: {0}") \
  X(TOAST_SPRITESHEET_NO_FRAMES, "No frames captured for spritesheet export.", "Ни один кадр не захвачен для экспорта спрайт-листа.", "导出图集需要捕获至少一帧.", "스프라이트 시트 내보내기용으로 캡처된 프레임이 없습니다.") \
  X(TOAST_UNDO, "Undo: {0}", "Отмена: {0}", "撤销: {0}", "실행 취소: {0}") \
  X(TOOLTIP_ADD_ANIMATION, "Add a new animation.", "Добавить новую анимацию.", "添加一个新动画.", "새 애니메이션을 추가합니다.") \
  X(TOOLTIP_ADD_EVENT, "Add an event.", "Добавить событие.", "添加一个事件.", "이벤트를 추가합니다.") \
  X(TOOLTIP_ADD_ITEM, "Add the item, with the settings specified.", "Добавить предмет с указанными настройками.", "按照指定的设置添加物品.", "지정된 설정으로 항목을 추가합니다.") \
  X(TOOLTIP_ADD_LAYER, "Add a layer.", "Добавить слой.", "添加一个动画层.", "레이어를 추가합니다.") \
  X(TOOLTIP_ADD_NULL, "Add a null.", "Добавить нуль.", "添加一个Null.", "Null을 추가합니다.") \
  X(TOOLTIP_ADD_SPRITESHEET, "Add a new spritesheet.", "Добавить новый спрайт-лист.", "添加一个新图集.", "새 스프라이트 시트를 추가합니다.") \
  X(TOOLTIP_ADD_VALUES, "Add the specified values onto each frame.\n(Boolean values will simply be set.)", "Добавить указанные значения к каждому кадру.\n(Булевы значения будут просто установлены.)", "将指定的数值添加到每一帧上. (任何布尔值[真假值]将直接被设置.)", "각 프레임의 속성에 지정한 값을 더합니다.\n(참/거짓 값은 그대로 설정됩니다.)") \
  X(TOOLTIP_ADJUST, "Set the value of each specified value onto the frame's equivalent.", "Установить значение каждого указанного значения к эквиваленту кадра.", "将每个指定的数值设置到对应帧的相等的属性上.", "지정된 각 값을 프레임의 대응 값으로 설정합니다.") \
  X(TOOLTIP_ALL_ITEMS_VISIBLE, "All items are visible. Press to only show layers.", "Все предметы видимы. Нажмите, чтобы только показать слои.", "所有物品均可见. 点击即可仅显示动画层.", "모든 항목이 표시됩니다. 레이어만 표시하려면 누르세요.") \
  X(TOOLTIP_ALT_ICONS, "Toggle a different appearance of the target icons.", "Переключить альтернативный вид иконок-перекрестий.", "切换指定图标为另一个样式.", "대상 아이콘을 다른 외형으로 전환합니다.") \
  X(TOOLTIP_ANIMATION_LENGTH, "Set the animation's length.", "Установить продолжительность анимации.", "设置动画时长.", "애니메이션의 길이를 설정합니다.") \
  X(TOOLTIP_AUTHOR, "Set the author of the document.", "Установить автора документа.", "设置文件的制作者.", "파일의 작성자를 설정합니다.") \
  X(TOOLTIP_AUTOSAVE_ENABLED, "Enables autosaving of documents.\n(Does not overwrite files; makes copies to restore later.)", "Включает автосохранение документов.\n(Не заменяет файлы; создает копии для последующего восстановления.)", "开启自动保存文件.\n(并不会覆盖原本的文件, 反而会创造出可重新恢复的新文件)", "파일 자동저장을 활성화합니다.\n(파일을 덮어쓰지 않습니다; 나중에 복원할 복사본을 만듭니다.)") \
  X(TOOLTIP_AUTOSAVE_INTERVAL, "If changed, will autosave documents using this interval.", "Если изменено, будет автосохранять документы используя этот промежуток.", "如果更改了, 那么会使自动保存使用这个时间区间.", "이 간격으로 파일을 자동저장합니다.") \
  X(TOOLTIP_AXES, "Toggle the axes' visibility.", "Переключить видимость осей.", "切换坐标轴是否可见.", "가로/세로 축을 표시하거나 숨깁니다.") \
  X(TOOLTIP_AXES_COLOR, "Set the color of the axes.", "Установить цвет осей.", "设置坐标轴的颜色.", "가로/세로 축의 색상을 설정합니다.") \
  X(TOOLTIP_BACKGROUND_COLOR, "Change the background color.", "Изменить цвет фона.", "更改背景颜色.", "배경색을 변경합니다.") \
  X(TOOLTIP_BAKE_FRAMES, "Turn interpolated frames into uninterpolated ones.\nUse the shortcut to bake frames quickly.", "Превратить интерполированные кадры в неинтерполированные.\nИспользуйте горячую клавишу, чтобы быстро запечь кадры.", "将线性插值的帧转换为普通帧。\n使用快捷键可快速烘焙帧.", "연결된 프레임을 연결되지 않은 프레임으로 고정화합니다.\n단축키를 사용하면 프레임을 빠르게 베이크할 수 있습니다.") \
  X(TOOLTIP_BAKE_FRAMES_OPTIONS, "Bake the selected frame(s) with the options selected.", "Запечь выбранные кадры с выбранными настройками.", "替换所选旧图集为新图集.", "선택된 프레임을 선택한 옵션으로 베이킹합니다.") \
  X(TOOLTIP_BORDER, "Toggle the visibility of borders around layers.", "Переключить видимость границ около слоев.", "切换动画层边框是否可见.", "레이어 주변 경계선을 표시하거나 숨깁니다.") \
  X(TOOLTIP_CANCEL_ADD_ITEM, "Cancel adding an item.", "Отменить добавление предмета.", "取消添加物品.", "항목 추가를 취소합니다.") \
  X(TOOLTIP_CANCEL_BAKE_FRAMES, "Cancel baking frames.", "Отменить запечку кадров.", "取消提前渲染.", "프레임 베이킹을 취소합니다.") \
  X(TOOLTIP_CENTER_VIEW, "Centers the view.", "Центрирует вид.", "居中视角.", "미리보기 화면을 가운데에 맞춥니다.") \
  X(TOOLTIP_CLOSE_SETTINGS, "Close without updating settings.", "Закрыть без обновления настройки.", "关闭但不保存设置.", "설정을 갱신하지 않고 닫습니다.") \
  X(TOOLTIP_COLOR_OFFSET, "Change the color added onto the frame.", "Изменить цвет, который добавлен на кадр.", "更改覆盖在帧上的颜色.", "프레임에 더해지는 색을 변경합니다.") \
  X(TOOLTIP_COLUMNS, "Set how many columns the spritesheet will have.", "Установить сколько колонн будет иметь спрайт-лист.", "设置图集有多少列.", "스프라이트 시트의 열 수를 설정합니다.") \
  X(TOOLTIP_CROP, "Change the crop position the frame uses.", "Изменить позицию обрезки, которую использует кадр.", "更改当前帧的裁剪位置.", "프레임에 대응되는 스프라이트 시트를 어느 지점부터 사용할지 변경합니다.") \
  X(TOOLTIP_CUSTOM_RANGE, "Toggle using a custom range for the animation.", "Переключить использование пользовательского диапазона для анимации.", "切换是否让动画使用自定义区间.", "애니메이션에 사용자 지정 길이를 사용할지 정합니다.") \
  X(TOOLTIP_DELETE_ANIMATIONS_AFTER, "Delete animations after merging them.", "Удалить анимации после их соединения.", "合并动画后，删除其他动画。", "병합 후 기존 애니메이션을 삭제합니다.") \
  X(TOOLTIP_DELETE_FRAMES, "Delete the selected frames.", "Удалить выбранные кадры.", "删除所选帧.", "선택된 프레임을 삭제합니다.") \
  X(TOOLTIP_DIVIDE_VALUES, "Divide the specified values for each frame.\n(Boolean values will simply be set.)", "Разделить указанные значения для каждого кадра.\n(Булевы значения будут просто установлены.)", "将每一帧的指定值进行除法操作. (布尔值将直接被设置.)", "각 프레임의 속성을 지정된 값으로 나눕니다.\n(참/거짓 값은 그대로 설정됩니다.)") \
  X(TOOLTIP_DUPLICATE_ANIMATION, "Duplicate the selected animation(s).", "Дублировать выбранные анимации.", "拷贝所选动画.", "선택된 애니메이션을 복제합니다.") \
  X(TOOLTIP_DURATION, "Change how long the frame lasts.", "Изменить сколько длится кадр.", "更改此帧的时长.", "프레임의 지속 시간을 변경합니다.") \
  X(TOOLTIP_EDITOR_ZOOM, "Change the zoom of the editor.", "Изменить масштаб редактора.", "更改编辑器的视角缩放.", "편집기의 줌을 변경합니다.") \
  X(TOOLTIP_END, "Set the ending time of the animation.", "Установить конечное время анимации.", "更改动画的结束时间.", "애니메이션의 종료 시간을 설정합니다.") \
  X(TOOLTIP_FFMPEG_PATH, "Set the path where the FFmpeg installation is located.\nFFmpeg is required to render animations.\nhttps://ffmpeg.org", "Установить путь, где находится установка FFmpeg.\nFFmpeg нужен для рендеринга анимаций.\nhttps://ffmpeg.org", "设置 FFmpeg 的安装路径.\n渲染动画需要 FFmpeg.\nhttps://ffmpeg.org", "FFmpeg 설치 경로를 설정합니다.\n애니메이션 렌더링에는 FFmpeg가 필요합니다.\nhttps://ffmpeg.org") \
  X(TOOLTIP_FIT, "Set the view to match the extent of the animation.", "Установить отображение чтобы оно соответствовало длительности анимации.", "使当前视角符合当前动画的长宽.", "미리보기 화면을 애니메이션 영역에 맞춥니다.") \
  X(TOOLTIP_FIT_ANIMATION_LENGTH, "The animation length will be set to the effective length of the animation.", "Длительность анимации будет установлена в соответствии с фактической длиной анимации.", "动画的当前时长将被设置为动画的有效时长.", "애니메이션 길이가 애니메이션의 유효 길이로 설정됩니다.") \
  X(TOOLTIP_FLIP_X, "Flip the X scale of the frame, to cheat mirroring the frame horizontally.\n(Note: the format does not support mirroring.)", "Отразить масштаб кадра по оси X, вместо отражения кадра горизонтально.\n(Примечание: формат не поддерживает нормальное отражение.)", "通过翻转X轴的缩放，使此帧看起来像X轴翻转了.\n(注: 此格式不支持镜像.)", "프레임의 가로 비율을 반전시켜 프레임이 수평으로 뒤집은 것처럼 보이게 합니다.\n(참고: 완전한 뒤집기 기능을 지원하지 않습니다.)") \
  X(TOOLTIP_FLIP_Y, "Flip the Y scale of the frame, to cheat mirroring the frame vertically.\n(Note: the format does not support mirroring.)", "Отразить масштаб кадра по оси Y, вместо отражения кадра вертикально.\n(Примечание: формат не поддерживает нормальное отражение.)", "通过翻转Y轴的缩放，使此帧看起来像Y轴翻转了.\n(注: 此格式不支持镜像.)", "프레임의 세로 비율을 반전시켜 프레임이 수직으로 뒤집은 것처럼 보이게 합니다.\n(참고: 완전한 뒤집기 기능을 지원하지 않습니다.)") \
  X(TOOLTIP_FORMAT, "For outputted images, each image will use this format.\n{0} represents the index of each image.", "Для выведенных изображений, каждое будет использовать этот формат.\n{} представляет индекс каждого изображения.", "用于输出的图像, 每一个图像都会使用这个格式.\n{} 代表每一个图像的下标.", "출력되는 이미지들은 이 형식을 사용합니다.\n{}는 각 이미지의 인덱스를 나타냅니다.") \
  X(TOOLTIP_FPS, "Set the FPS of all animations.", "Установить сколько кадров в секунде для всех анимаций.", "设置所有动画的FPS(帧数每秒)", "모든 애니메이션의 FPS를 설정합니다.") \
  X(TOOLTIP_FRAME_INTERPOLATION, "Toggle the frame interpolating; i.e., blending its values into the next frame based on the time.", "Переключить интерполяцию кадра; т. е. смешать его значения в следующий кадр на основе времени.", "切换帧的线性插值; 也就是利用时间来\"渐变\"两帧之间的数值.", "프레임 보간(시간이 따라 속성 값이 다음 프레임의 값으로 변함) 여부를 정합니다.") \
  X(TOOLTIP_FRAME_VISIBILITY, "Toggle the frame's visibility.", "Переключить видимость кадра.", "切换此帧是否可见.", "프레임을 표시하거나 숨깁니다.") \
  X(TOOLTIP_GLOBAL_LOCALE, "Use the document's global locale.", "Использовать глобальный локаль документа.", "使用当前文件的全局本地化.", "파일의 전역 경로를 사용합니다.") \
  X(TOOLTIP_GRID_COLOR, "Change the grid's color.", "Изменить цвет сетки.", "更改网格的颜色.", "격자 색을 변경합니다.") \
  X(TOOLTIP_GRID_OFFSET, "Change the offset of the grid.", "Изменить смещение сетки.", "更改网格的偏移量.", "격자의 오프셋을 변경합니다.") \
  X(TOOLTIP_GRID_SIZE, "Change the size of all cells in the grid.", "Изменить размер всех клеток в сетке.", "更改网格中所有单元格的大小。", "격자의 간격을 변경합니다.") \
  X(TOOLTIP_GRID_SNAP, "Cropping will snap points to the grid.", "Обрезание привяжет точки к сетке.", "裁剪时会吸附到网格上.", "자르는 중점이 격자에 맞게 정렬됩니다.") \
  X(TOOLTIP_GRID_VISIBILITY, "Toggle the visibility of the grid.", "Переключить видимость сетки.", "切换网格是否可见.", "그리드의 표시 여부를 전환합니다.") \
  X(TOOLTIP_INSERT_FRAME, "Insert a frame, based on the current selection.", "Вставить кадр на основе текущего выбора.", "按照当前的选择去插入帧.", "현재 선택을 기반으로 프레임을 삽입합니다.") \
  X(TOOLTIP_INTERVAL, "Set the maximum duration of each frame that will be baked.", "Установить максимальную длительносто каждого кадра, который будет запечен.", "更改提前渲染时, 每一帧的最大时长.", "베이크될 각 프레임의 최대 지속 시간을 설정합니다.") \
  X(TOOLTIP_ITEM_NAME, "Set the item's name.", "Назвать предмет.", "设置物品的名称.", "항목의 이름을 설정합니다.") \
  X(TOOLTIP_ITEM_VISIBILITY_HIDDEN, "The item is hidden. Press to show it.", "Этот предмет скрыт. Нажмите, чтобы его показать.", "此物品已隐藏. 点击以显示.", "항목이 숨겨져 있습니다. 표시하려면 누르세요.") \
  X(TOOLTIP_ITEM_VISIBILITY_SHOWN, "The item is visible. Press to hide it.", "Этот предмет видим. Нажмите, чтобы его скрыть.", "此物品当前可见. 点击以隐藏.", "항목이 표시되어 있습니다. 숨기려면 누르세요.") \
  X(TOOLTIP_LANGUAGE, "Change the language of the program.", "Изменить язык программы.", "修改程序的语言.", "프로그램의 언어를 변경합니다.") \
  X(TOOLTIP_LAYER_SPRITESHEET, "Set the layer item's spritesheet.", "Установить спрайт-лист слоевого предмета.", "设置层物品的图集.", "레이어 항목의 스프라이트 시트를 설정합니다.") \
  X(TOOLTIP_LAYER_TYPE, "Add a layer item.", "Использовать размер обрезки, который использует кадр.", "添加层物品.", "레이어 항목을 추가합니다.") \
  X(TOOLTIP_LOCAL_LOCALE, "Use a local locale for the item.", "Изпользовать локальный локаль для предмета.", "让此物品使用局部本地化.", "항목에 지역 경로를 사용합니다.") \
  X(TOOLTIP_LOOP_ANIMATION, "Toggle the animation looping.", "Переключить цикличное возпроизведение анимации.", "切换动画是否循环.", "애니메이션을 반복할지 정합니다.") \
  X(TOOLTIP_MOVE_TOOL_SNAP, "In Animation Preview, the Move tool will snap the frame's position right to the cursor, instead of being moved at a distance.", "В предпросмотре анимации, инструмент передвижения привяжет позицию кадра прямо к курсору, а не перемещает его на расстоянии.", "在动画预览时, 移动工具会使帧的位置直接吸附与光标上，而不是按距离移动.", "애니메이션 미리보기에서 이동 도구로 프레임을 이동시킬 때 프레임을 마우스 커서 바로 옆에 정렬되게 합니다.") \
  X(TOOLTIP_MULTIPLY_VALUES, "Multiply the specified values for each frame.\n(Boolean values will simply be set.)", "Умножить указанные значения для каждого кадра.\n(Булевы значения будут просто установлены.)", "将每一帧的指定值进行相乘操作. (布尔值将直接被设置.)", "각 프레임의 속성에 지정한 값을 곱합니다.\n(참/거짓 값은 그대로 설정됩니다.)") \
  X(TOOLTIP_NEW_ITEM, "Create a new item.", "Создать новый предмет.", "创造一个新物品.", "새 항목을 만듭니다.") \
  X(TOOLTIP_NO_UNUSED_ITEMS, "There are no unused items to use.", "Нет неиспользуемых предметов, которые использовать.", "没有可用的未使用物品.", "사용하지 않는 항목이 없습니다.") \
  X(TOOLTIP_NULL_NAME, "Set the null's name.", "Назвать этот нуль.", "更改Null的名字.", "Null의 이름을 설정합니다.") \
  X(TOOLTIP_NULL_RECT, "The null will have a rectangle show around it.", "Около этого нуля будет прямоугольник.", "此Null周围会显示一个方框.", "Null의 주변에 사각형이 표시됩니다.") \
  X(TOOLTIP_NULL_RECT_HIDDEN, "The null's rectangle is hidden. Press to show it.", "Прямоугольник этого нуля скрыт. Нажмите, чтобы его показать.", "此Null的方框已隐藏. 点击以显示.", "Null의 사각형이 숨겨져 있습니다. 표시하려면 누르세요.") \
  X(TOOLTIP_NULL_RECT_SHOWN, "The null's rectangle is shown. Press to hide it.", "Прямоугольник этого нуля видим. Нажмите, чтобы его скрыть.", "此Null的方框当前可见. 点击以隐藏.", "Null의 사각형이 표시되어 있습니다. 숨기려면 누르세요.") \
  X(TOOLTIP_NULL_TYPE, "Add a null item.", "Добавить нулевой предмет.", "添加一个Null物品.", "Null 항목을 추가합니다.") \
  X(TOOLTIP_ONIONSKIN_COLOR, "Change the color this onionskin will use.", "Изменить цвет, который использует оньонскин.", "更改此洋葱皮预览的颜色.", "전후 비교에서 사용할 색을 변경합니다.") \
  X(TOOLTIP_ONIONSKIN_ENABLED, "Toggle onionskinning.", "Переключить оньонскин.", "切换是否使用洋葱皮预览.", "전후 비교로 전환합니다.") \
  X(TOOLTIP_ONIONSKIN_FRAMES, "Change the amount of frames this onionskin will use.", "Изменить кол-во кадров, которое будет использовать этот оньонскин.", "更改洋葱皮预览使用的总帧数.", "전후 비교에서 사용할 프레임 수를 변경합니다.") \
  X(TOOLTIP_ONIONSKIN_INDEX, "The onionskinned frames will be based on frame index.", "Кадры оньонскина будут основаны на индексе кадров.", "洋葱皮预览的帧会基于帧下标.", "프레임 비교를 프레임 인덱스를 기준으로 합니다.") \
  X(TOOLTIP_ONIONSKIN_TIME, "The onionskinned frames will be based on frame time.", "Кадры оньонскина будут основаны на времени кадров.", "洋葱皮预览的帧会基于帧时间.", "프레임 비교를 프레임 시간을 기준으로 합니다.") \
  X(TOOLTIP_ONLY_LAYERS_VISIBLE, "Only layers are visible. Press to show all items.", "Только слои видимы. Нажмите, чтобы показать все предметы.", "当前仅有动画层可见. 点击以显示所有物品.", "레이어만 표시합니다. 모두 보려면 누르세요.") \
  X(TOOLTIP_OPEN_MERGE_POPUP, "Open merge popup.\nUse the shortcut to merge quickly.", "Открыть всплывающее окно соединения.\nИспользуйте горячую клавишу, чтобы быстро выполнить слияние.", "打开合并弹窗。\n使用快捷键可快速合并。", "병합 팝업을 엽니다.\n단축키로 빠르게 병합하세요.") \
  X(TOOLTIP_OUTPUT_PATH, "Set the output path or directory for the animation.", "Установить путь или директорию вывода для анимации.", "更改动画的输出路径/目录.", "애니메이션의 출력 경로 또는 디렉터리를 설정합니다.") \
  X(TOOLTIP_OVERLAY, "Set an animation to be drawn over the current animation.", "Установить анимацию, которая будет выведена над текущей анимацией.", "设置一个当前动画的覆盖动画.", "현재 애니메이션 위에 그려질 애니메이션을 설정합니다.") \
  X(TOOLTIP_OVERLAY_ALPHA, "Set the alpha of the overlayed animation.", "Установить прозрачность наложенной анимации.", "更改覆盖动画的透明度.", "오버레이된 애니메이션의 불투명도를 설정합니다.") \
  X(TOOLTIP_OVERWRITE_WARNING, "A warning will be shown when saving/overwriting a file.", "При сохранении/перезаписи файла будет показано предупреждение.", "保存或覆盖文件时会提示确认。", "저장하거나 덮어쓸 때 경고가 표시됩니다.") \
  X(TOOLTIP_PAUSE_ANIMATION, "Pause the animation.", "Поставить анимацию на паузу.", "暂停动画.", "애니메이션을 일시정지합니다.") \
  X(TOOLTIP_PIVOT, "Change the pivot of the frame; i.e., where it is centered.", "Изменить точку поворота кадра, т. е. где он центрирован.", "更改帧枢轴; 比如可以让帧枢轴居中.", "프레임의 중심점을 변경합니다.") \
  X(TOOLTIP_PIVOTS, "Toggle the visibility of the animation's pivots.", "Переключить видимость точек поворота анимации.", "切换动画枢轴是否可见.", "애니메이션의 중심점을 표시하거나 숨깁니다.") \
  X(TOOLTIP_PLAYBACK_ALWAYS_LOOP, "Animations will always loop during playback, even if looping isn't set.", "Анимации всегда будут циклично воспроизводиться во время воспроизведения, даже если цикличность не установлена.", "动画预放时也会循环, 即使循环并未被设置.", "반복 설정이 켜져 있지 않아도 애니메이션이 항상 반복 재생됩니다.") \
  X(TOOLTIP_PLAYBACK_CLAMP, "Operations will always be clamped to within the animation's bounds.\nFor example, dragging the playhead, or triggers.", "Операции всегда будут ограничены границами анимации.\nНапример, перетаскивание воспроизводящей головки или триггеров.", "任何操作都会被限制在动画的区间内.\n比如, 拖拽播放指针或者触发器.", "작업이 항상 애니메이션 범위 내로 제한됩니다.\n예: 플레이헤드를 드래그하거나 트리거를 조작할 때.") \
  X(TOOLTIP_PLAY_ANIMATION, "Play the animation.", "Возпроизвести анимацию.", "播放动画.", "애니메이션을 재생합니다.") \
  X(TOOLTIP_POSITION, "Change the position of the frame.", "Изменить позицию кадра.", "更改此帧的位置.", "프레임의 위치를 변경합니다.") \
  X(TOOLTIP_PREVIEW_ZOOM, "Change the zoom of the preview.", "Изменить масштаб предпросмотра.", "更改预览视图的缩放.", "미리보기의 줌을 변경합니다.") \
  X(TOOLTIP_RAW, "Record only the raw animation; i.e., only its layers, to its bounds.", "Записывать только «сырую» анимацию, т. е. только ее слои.", "仅保存原生动画; 比如只保存动画层.", "Raw 애니메이션만 녹화합니다. 즉, 레이어만 녹화합니다.") \
  X(TOOLTIP_RELOAD_SPRITESHEETS, "Reloads the selected spritesheets.", "Перезагружает выбранные спрайт-листы.", "重新加载所选图集.", "선택한 스프라이트 시트를 다시 불러옵니다.") \
  X(TOOLTIP_REMOVE_ANIMATION, "Remove the selected animation(s).", "Удалить выбранные анимации.", "去除所选动画.", "선택한 애니메이션을 제거합니다.") \
  X(TOOLTIP_REMOVE_ITEMS, "Remove the selected item(s).", "Удалить выбранные предметы.", "去除所选物品.", "선택한 항목을 제거합니다.") \
  X(TOOLTIP_REMOVE_UNUSED_EVENTS, "Remove unused events (i.e., ones not used by any trigger in any animation.)", "Удалить неиспользуемые события (т. е. события, которые не использует ни один триггер в ни одной анимации.)", "去除未使用的事件 (未被任何动画触发的事件.)", "사용되지 않는 이벤트(어떤 애니메이션의 트리거에서도 사용되지 않는 것)를 제거합니다.") \
  X(TOOLTIP_REMOVE_UNUSED_LAYERS, "Remove unused layers (i.e., ones not used in any animation.)", "Удалить неиспользуемые слои (т. е. слои, которые не используются ни одной анимацией.)", "去除未使用的动画层 (未被任何动画使用的那些)", "사용되지 않는 레이어(어떤 애니메이션에서도 사용되지 않는 것)를 제거합니다.") \
  X(TOOLTIP_REMOVE_UNUSED_NULLS, "Remove unused nulls (i.e., ones not used in any animation.)", "Удалить неиспользуемые нули (т. е. нули, которые не используются ни одной анимацией.)", "去除未使用的Null (未被任何动画使用的那些.)", "사용되지 않는 Null(어떤 애니메이션에서도 사용되지 않는 것)을 제거합니다.") \
  X(TOOLTIP_REMOVE_UNUSED_SOUNDS, "Remove unused sounds (i.e., ones not used in any trigger.)", "Удалить неиспользуемые звуки (т. е. звуки, которые не используются ни одным триггером.)", "去除未使用的声音 (未被任何触发器使用的那些.)", "사용되지 않는 사운드(어떤 트리거에서도 사용되지 않는 것)를 제거합니다.") \
  X(TOOLTIP_REMOVE_UNUSED_SPRITESHEETS, "Remove all unused spritesheets (i.e., not used in any layer.).", "Удалить неиспользуемые спрайт-листы (т. е. те, которые не используются ни одним слоем.)", "去除未使用的图集 (未被任何动画层使用的那些.)", "사용되지 않는 모든 스프라이트 시트(어떤 레이어에서도 사용되지 않는 것)를 제거합니다.") \
  X(TOOLTIP_RENDER_BUTTON, "Render the animation using the current settings.", "Рендерировать анимацию, используя текущие настройки.", "应用当前设置去渲染动画.", "현재 설정으로 애니메이션을 렌더링합니다.") \
  X(TOOLTIP_RENDER_TYPE, "Set the type of the output.", "Установить тип вывода.", "设置输出的类型.", "출력 유형을 설정합니다.") \
  X(TOOLTIP_REPEAT_DELAY, "Set how often, after repeating begins, key inputs will be fired.", "Установить, как часто после начала повторения будут срабатывать нажатия клавиш.", "更改键盘按键开启重复时, 重复的速率.", "반복 입력이 시작된 후 입력이 얼마나 자주 들어갈지 설정합니다.") \
  X(TOOLTIP_REPLACE_SPRITESHEET, "Replace the selected spritesheet with a new one.", "Заменить выбранный спрайт-лист на новый.", "替换所选旧图集为新图集.", "선택된 스프라이트 시트를 새 시트로 교체합니다.") \
  X(TOOLTIP_ROOT_TRANSFORM, "Root frames will transform the rest of the animation.", "Корневые кадры трансформируют остаток анимации.", "与“根”有关的帧会跟随整个动画而变换.", "Root 프레임이 나머지 애니메이션을 변형합니다.") \
  X(TOOLTIP_ROTATION, "Change the rotation of the frame.", "Изменить поворот кадра.", "更改此帧的旋转.", "프레임의 회전값을 변경합니다.") \
  X(TOOLTIP_ROUND_ROTATION, "Rotation will be rounded to the nearest whole number.", "Поворот будет округлен к самому близкому целому числу.", "旋转数值会被取整.", "회전값을 가장 가까운 정수로 반올림합니다.") \
  X(TOOLTIP_ROUND_SCALE, "Scale will be rounded to the nearest whole number.", "Масштаб будет округлен к самому близкому целому числу.", "缩放数值会被取整.", "비율을 가장 가까운 정수로 반올림합니다.") \
  X(TOOLTIP_ROWS, "Set how many rows the spritesheet will have.", "Установить сколько рядов будет иметь спрайт-лист.", "设置图集的行数量.", "스프라이트 시트의 행 수를 설정합니다.") \
  X(TOOLTIP_SAVE_SPRITESHEETS, "Save the selected spritesheets.", "Сохранить выбранные спрайт-листы.", "保存所选图集.", "선택한 스프라이트 시트를 저장합니다.") \
  X(TOOLTIP_SCALE, "Change the scale of the frame, in percent.", "Изменить масштаб кадра.", "修改此帧的缩放.", "프레임의 비율을 변경합니다.") \
  X(TOOLTIP_SCALE_OUTPUT, "Set the output scale of the animation.", "Установить масштаб анимации при выводе.", "设置输出动画的缩放.", "애니메이션의 출력 비율을 설정합니다.") \
  X(TOOLTIP_SETTINGS_SAVE, "Use the configured settings.", "Использовать настроенные настройки.", "应用更改的设置.", "구성된 설정을 사용합니다.") \
  X(TOOLTIP_SET_DEFAULT_ANIMATION, "Set the selected animation as the default.", "Установить выбранную анимацию как анимацию по умолчанию.", "把当前所选动画设置为默认动画.", "선택한 애니메이션을 기본 애니메이션으로 설정합니다.") \
  X(TOOLTIP_SET_TO_RECOMMENDED, "Use a recommended value for rows/columns.", "Использовать рекомендованное значение для рядов/колонн.", "应用列/行的推荐值.", "행/열에 권장값을 사용합니다.") \
  X(TOOLTIP_SPLIT, "Based on the playhead time, split the selected frame into two.", "С учётом позиции ползунка воспроизведения разделяет выбранный кадр на два.", "根据播放头位置，将所选帧拆分成两个。", "재생 헤드 시간에 따라 선택한 프레임을 두 개로 분할합니다.") \
  X(TOOLTIP_SIZE, "Change the crop size the frame uses.", "Изменить размер обрезки, который использует этот кадр.", "更改此帧的裁剪大小.", "프레임에 대응되는 스프라이트 시트의 사용 영역의 크기를 변경합니다.") \
  X(TOOLTIP_SOUND, "Toggle sounds playing with triggers.\nBind sounds to events in the Events window.", "Переключить воспроизведения звуков с помощью триггеров.\nПривязывайте звуки к событиям в окне событий.", "切换是否在触发器触发时播放声音.\n可以在事件窗口里链接声音与事件.", "트리거와 함께 사운드를 재생할지 정합니다.\n사운드는 이벤트 창에서 이벤트에 연결하세요.") \
  X(TOOLTIP_SOUNDS_PLAY, "Click to play.", "Нажмите, чтобы возпроизвести.", "点击播放.", "클릭하여 재생합니다.") \
  X(TOOLTIP_SOUND_INVALID, "This sound could not be loaded. Replace the file.", "Этот звук не удалось загрузить. Замените файл.", "无法加载此声音。请替换文件。", "이 사운드를 불러올 수 없습니다. 파일을 교체하세요.") \
  X(TOOLTIP_SOUND_ADD, "Add a sound.", "Добавить звук.", "添加一个声音.", "사운드를 추가합니다.") \
  X(TOOLTIP_SPRITESHEET_BORDER, "Toggle a border appearing around the spritesheet.", "Переключить показ границ около спрайт-листа.", "切换是否显示在图集的边框.", "스프라이트 시트 주변에 경계선을 표시하거나 숨깁니다.") \
  X(TOOLTIP_SPRITESHEET_INVALID, "This spritesheet isn't valid!\nLoad an existing, valid texture.", "Этот спрайт-лист невалиден!\nЗагрузите существующую, валидную текстуру.", "此图集无效!\n请加载一个已存在并有效的纹理/图集.", "이 스프라이트 시트는 유효하지 않습니다!\n유효한 텍스처를 불러오세요.") \
  X(TOOLTIP_STACK_SIZE, "Set the maximum snapshot stack size of a document (i.e., how many undo/redos are preserved at a time).", "Установить максимальный размер стека снимков документа (т. е. количество отмен/повторов, сохраняемых одновременно).", "设置文件的快照栈的最大存储空间. (也就是最大可以存储多少撤销与重做)", "파일의 최대 스냅숏 스택 크기(즉 한 번에 보존되는 실행 취소/다시 실행 수)를 설정합니다.") \
  X(TOOLTIP_START, "Set the starting time of the animation.", "Установить начальное время анимации.", "设置动画的起始时间.", "애니메이션의 시작 시간을 설정합니다.") \
  X(TOOLTIP_SUBTRACT_VALUES, "Subtract the specified values from each frame.\n(Boolean values will simply be set.)", "Вычтить указанные значения из каждого кадра.\n(Булевы значения будут просто указаны.)", "将每一帧的指定值进行相减操作. (布尔值将直接被设置.)", "각 프레임의 속성에 지정한 값을 뺍니다.\n(참/거짓 값은 그냥 설정됩니다.)") \
  X(TOOLTIP_TIMELINE_SHORTCUTS, "- Press {0} to decrement time.\n- Press {1} to increment time.\n- Press {2} to shorten the selected frame, by one frame.\n- Press {3} to extend the selected frame, by one frame.\n- Press {4} to go to the previous frame.\n- Press {5} to go to the next frame.\n- Click and hold on a frame while holding CTRL to change its duration.\n- Click and hold on a trigger to change its At Frame.\n- Hold Alt while clicking a non-trigger frame to toggle interpolation.", "- Нажмите {0}, чтобы уменьшить время.\n- Нажмите {1}, чтобы увеличить время.\n- Нажмите {2}, чтобы укоротить выбранный кадр одной мерной единицей.\n- Нажмите {3}, чтобы продлить выбранный кадр на одну мерную единицу.\n- Нажмите {4}, чтобы перейти к предыдущему кадру.\n- Нажмите {5}, чтобы перейти к следующему кадру.\n- Удерживайте нажатой кнопку мыши по кадру, удерживая CTRL, чтобы изменить его длительность.\n- Нажмите и удерживайте кнопку мыши по триггеру, чтобы изменить параметр «На кадре».\n- Удерживайте Alt и нажмите по кадру, который не является триггером, чтобы переключить интерполяцию.", "- 按下 {0} 减少时间.\n- 按下 {1} 增加时间.\n- 按下 {2} 将所选帧缩短一帧.\n- 按下 {3} 将所选帧延长一帧.\n- 按下 {4} 跳到上一帧.\n- 按下 {5} 跳到下一帧.\n- 在按住 CTRL 的同时点击并按住某一帧以更改其持续时间.\n- 点击并按住触发器即可更改其触发帧.\n- 按住 Alt 并点击任何无触发器的帧以切换线性插值的使用.", "- {0} 키: 플레이헤드를 뒤로 보냅니다.\n- {1} 키: 플레이헤드를 앞으로 보냅니다.\n- {2} 키: 선택한 프레임을 한 프레임 단축합니다.\n- {3} 키: 선택한 프레임을 한 프레임 연장합니다.\n- {4} 키: 이전 프레임을 선택합니다.\n- {5} 키: 다음 프레임을 선택합니다.\n- CTRL 키를 누른 채 프레임을 클릭하고 드래그하면 프레임의 유지 시간을 변경할 수 있습니다.\n- 트리거를 클릭하고 드래그하면 트리거의 시작 프레임을 변경할 수 있습니다.\n- Alt 키를 누른 채 트리거를 제외한 프레임을 클릭하면 매끄럽게 연결 설정을 켤 수 있습니다.") \
  X(TOOLTIP_TINT, "Change the tint of the frame.", "Изменить оттенок кадра.", "更改此帧的色调.", "프레임의 색조를 변경합니다.") \
  X(TOOLTIP_TOOL_COLOR, "Selects the color to be used for drawing.\n(Spritesheet Editor only.)", "Выбирает цвет, который будет использоваться для рисования.\n(Только в редакторе спрайт-листов.)", "选择用于绘画的颜色.\n(仅应用于图集编辑器.)", "그리기용으로 사용할 색을 선택합니다.\n(스프라이트 시트 편집기 전용)") \
  X(TOOLTIP_TOOL_COLOR_PICKER, "Selects a color from the canvas.\n(Spritesheet Editor only.)", "Выбирает цвет из холста.\n(Только в редакторе спрайт-листов.)", "从画板上选择一个颜色.\n(仅应用于图集编辑器.)", "캔버스에서 색을 선택합니다.\n(스프라이트시트 편집기 전용)") \
  X(TOOLTIP_TOOL_CROP, "Use the crop tool.\nWill produce a crop rectangle based on how the cursor is dragged, or directional keys are pressed.\nHold CTRL with arrow keys to change position.\nHolding right click will use the Move tool's functionality.\n(Spritesheet Editor only.)", "Использовать инструмент обрезки.\nБудет создан прямоугольник обрезки в зависимости от перемещения курсора или нажатия клавиш со стрелками.\nУдерживайте клавишу CTRL вместе с клавишами со стрелками, чтобы изменить положение.\nУдерживание правой кнопки мыши позволит использовать функциональность инструмента перемещения.\n(Только в редакторе спрайт-листов.)", "使用裁剪工具.\n将根据光标的拖动方式或方向键的按下而出现一个裁剪矩形.\n按住 CTRL 加方向键可改变其位置.\n按住右键将使用移动工具的功能.\n(仅限图集编辑器.)", "자르기 도구를 사용합니다.\n커서를 드래그하거나 방향키를 눌러 자르기 사각형을 만듭니다.\n화살표 키와 CTRL 키를 같이 눌러 위치를 변경합니다.\n오른쪽 클릭을 누른 채 사용하면 이동 도구의 기능이 대신 사용됩니다.\n(스프라이트 시트 편집기 전용)") \
  X(TOOLTIP_TOOL_DRAW, "Draws pixels onto the selected spritesheet, with the current color.\n(Spritesheet Editor only.)", "Рисует пиксели на выбранный спрайт-лист текущим цветом.\n(Только в редакторе спрайт-листов.)", "使用当前的颜色在所选的图集上绘画像素.\n(仅应用于图集编辑器.)", "선택된 스프라이트 시트에 픽셀을 그립니다. 현재 색을 사용합니다.\n(스프라이트 시트 편집기 전용)") \
  X(TOOLTIP_TOOL_ERASE, "Erases pixels from the selected spritesheet.\n(Spritesheet Editor only.)", "Стирает пиксели из выбранного спрайт-листа.\n(Только в редакторе спрайт-листов.)", "从所选的图集上擦除像素.\n(仅应用于图集编辑器.)", "선택된 스프라이트 시트에서 픽셀을 지웁니다.\n(스프라이트 시트 편집기 전용)") \
  X(TOOLTIP_TOOL_MOVE, "Use the move tool.\nAnimation Preview: Will move the position of the frame.\nSpritesheet Editor: Will move the pivot, and holding right click will use the Crop functionality instead.\nUse mouse or directional keys to change the value.", "Изпользовать инструмент перемещения.\nВ предпросмотрe анимации: Переместит позицию кадра.\nВ редакторе спрайт-листов: Переместит точку поворота, а удерживая правой кнопкой мыши позволит использовать функциональность инструмента обрезки.\nИзпользуйте мышь или клавиши со стрелками, чтобы изменить значение.", "使用移动工具.\n动画预览：将移动此帧的位置.\n图集编辑器: 会移动枢轴, 按住右键则会改为使用裁剪功能.\n使用鼠标或方向键来更改其数值.", "이동 도구를 사용합니다.\n애니메이션 미리보기: 프레임의 위치를 이동합니다.\n스프라이트 시트 편집기: 중심점을 이동하며, 오른쪽 클릭을 누른 채 사용하면 자르기 도구의 기능이 대신 사용됩니다.\n값을 변경하려면 마우스나 방향키를 사용하세요.") \
  X(TOOLTIP_TOOL_PAN, "Use the pan tool.\nWill shift the view as the cursor is dragged.\nYou can also use the middle mouse button to pan at any time.", "Использовать инструмент панорамирования. Будет смещать вид при перемещении курсора.\nВы также можете использовать среднюю кнопку мыши для панорамирования в любое время.", "使用平移工具.\n将随着光标拖动来移动视角.\n你也可以随时使用鼠标中键进行平移.", "시점 이동 도구를 사용합니다.\n커서를 드래그하면 시점이 이동합니다.\n언제든지 중간 마우스 버튼으로도 이동할 수 있습니다.") \
  X(TOOLTIP_TOOL_REDO, "Redoes the last action.", "Повторяет последнее действие.", "重做上一个操作.", "마지막 작업을 다시 실행합니다.") \
  X(TOOLTIP_TOOL_ROTATE, "Use the rotate tool.\nWill rotate the selected item as the cursor is dragged, or directional keys are pressed.\n(Animation Preview only.)", "Использовать инструмент поворота.\nПовернет выбранный предмет при перемещении курсора или при нажатии клавишей со стрелками.\n(Только в предпросмотре анимации.)", "使用旋转工具.\n将随着光标拖动或方向键按下来旋转选中的物品.\n(仅应用于动画预览.)", "회전 도구를 사용합니다.\n커서를 드래그하거나 방향키를 눌러 선택한 항목을 회전합니다.\n(애니메이션 미리보기 전용)") \
  X(TOOLTIP_TOOL_SCALE, "Use the scale tool.\nWill scale the selected item as the cursor is dragged, or directional keys are pressed.\nHold SHIFT to lock scaling to one dimension.\n(Animation Preview only.)", "Использовать инструмент масштабирования.\nВыбранный элемент будет масштабироваться при перетаскивании курсора или нажатии клавиш со стрелками.\nУдерживайте клавишу SHIFT, чтобы заблокировать масштабирование в одном измерении.\n(Только в предпросмотре анимации.)", "使用缩放工具.\n将随着光标拖动或方向键按下来缩放选中的物品.\n按住 SHIFT 可将缩放锁定为单一维度(单一坐标轴).\n(仅应用与动画预览.)", "비율 도구를 사용합니다.\n커서를 드래그하거나 방향키를 눌러 선택한 항목의 비율을 조정합니다.\nSHIFT 키를 누르면 한 축으로 고정됩니다.\n(애니메이션 미리보기 전용)") \
  X(TOOLTIP_TOOL_UNDO, "Undoes the last action.", "Отменяет последнее действие.", "重做上一个操作.", "마지막 작업을 취소합니다.") \
  X(TOOLTIP_TO_ANIMATION_RANGE, "Set the range to the normal range of the animation.", "Установить диапазон равным обычному диапазону анимации.", "将范围设置为动画的正常范围.", "렌더링 범위를 애니메이션의 범위로 설정합니다.") \
  X(TOOLTIP_TO_SELECTED_FRAMES, "If frames are selected, use that range for the rendered animation.", "Если кадры выбраны, использовать этот диапазон для рендерированной анимации.", "只渲染选择范围内的帧.", "프레임이 선택된 경우 그 영역만 렌더링 범위로 사용합니다.") \
  X(TOOLTIP_TRANSPARENT, "Toggle the spritesheet editor being transparent.", "Переключить прозрачность спрайт-листа.", "切换图集编辑器是否透明.", "스프라이트 시트 편집기를 투명하게 전환합니다.") \
  X(TOOLTIP_TRIGGER_AT_FRAME, "Change the frame the trigger will be activated at.", "Изменить кадр, на котором будет активироваться триггер.", "更改此触发器的触发帧.", "트리거가 활성화될 프레임을 변경합니다.") \
  X(TOOLTIP_TRIGGER_EVENT, "Change the event this trigger uses.", "Изменить событие, которое использует этот триггер.", "更改此触发器使用的事件.", "이 트리거가 사용할 이벤트를 변경합니다.") \
  X(TOOLTIP_TRIGGER_SOUND, "Change the sound this trigger uses.", "Изменить звук, который использует этот триггер.", "更改此触发器使用的声音.", "이 트리거가 사용할 사운드를 변경합니다.") \
  X(TOOLTIP_TRIGGER_VISIBILITY, "Toggle the trigger's visibility.", "Переключить видимость триггера.", "切换触发器是否可见.", "트리거를 표시하거나 숨깁니다.") \
  X(TOOLTIP_UI_SCALE, "Change the scale of the UI.", "Изменить масштаб пользовательского интерфейса.", "更改界面(UI)的缩放.", "UI 비율을 변경합니다.") \
  X(TOOLTIP_UNUSED_ITEMS_HIDDEN, "Unused layers/nulls are hidden. Press to show them.", "Неиспользуемые слои/нули скрыты. Нажмите, чтобы их показать.", "正在隐藏未使用的动画层/Null. 点击以显示它们.", "사용되지 않는 레이어/Null이 숨겨져 있습니다. 표시하려면 누르세요.") \
  X(TOOLTIP_UNUSED_ITEMS_SHOWN, "Unused layers/nulls are shown. Press to hide them.", "Неиспользуемые слои/нули видимы. Нажмите, чтобы их скрыть.", "正在显示未使用的动画层/Null. 点击以隐藏它们.", "사용되지 않는 레이어/Null이 표시되어 있습니다. 숨기려면 누르세요.") \
  X(TOOLTIP_USE_DEFAULT_SETTINGS, "Reset the settings to their defaults.", "Сбросить настройки на настройки по умолчанию.", "重设所有设置为默认.", "설정을 기본값으로 재설정합니다.") \
  X(TOOLTIP_USE_EXISTING_ITEM, "Reuse an unused item instead of creating a new one.", "Использовать неиспользуемый предмет, а не создавать новый.", "重用未使用的物品，而不是创建新物品。", "새로 만들지 않고 사용되지 않는 항목을 재사용합니다.") \
  X(TOOLTIP_VSYNC, "Toggle vertical sync; synchronizes program update rate with monitor refresh rate.", "Переключить вертикальную синхронизацию; синхронизирует частоту обновления программы с частотой обновления монитора.", "切换垂直同步; 同步程序的更新频率与屏幕的刷新频率.", "수직 동기화를 켜거나 끕니다. 프로그램의 업데이트 속도를 모니터의 새로고침 속도와 동기화합니다.") \
  X(TOOLTIP_ZOOM_STEP, "When zooming in/out with mouse or shortcut, this value will be used.", "При масштабировании мышью или горячей клавишей будет использоваться это значение.", "当通过鼠标或快捷键放大/缩小视图时, 此数值会被使用.", "마우스나 단축키로 확대/축소할 때 이 값이 사용됩니다.")
  // clang-format on

  enum StringType
  {
#define X(symbol, ...) symbol,
    STRINGS(X)
#undef X
        STRING_TYPE_COUNT
  };

#define DEFINE_LANGUAGE_ARRAY(symbol, label, selector)                                                                 \
  static constexpr const char* STRINGS_##symbol[] = {STRINGS(selector)};
  LANGUAGES(DEFINE_LANGUAGE_ARRAY)
#undef DEFINE_LANGUAGE_ARRAY

  static constexpr const char* const* LANGUAGE_STRING_COLLECTIONS[LANGUAGE_COUNT] = {
#define X(symbol, label, selector) STRINGS_##symbol,
      LANGUAGES(X)
#undef X
  };

#undef SELECT_ENGLISH
#undef SELECT_RUSSIAN
#undef SELECT_CHINESE
#undef SELECT_KOREAN
#undef STRINGS
#undef LANGUAGES

  class Localizer
  {
  public:
    Language language{ENGLISH};

    inline const char* get(StringType type, Language setLanguage = (Language)-1) const
    {
      auto useLanguage = (int)setLanguage == -1 ? language : setLanguage;
      if (useLanguage < 0 || useLanguage >= LANGUAGE_COUNT || type < 0 || type >= STRING_TYPE_COUNT) return "undefined";
      return LANGUAGE_STRING_COLLECTIONS[useLanguage][type];
    }
  };
}

extern anm2ed::Localizer localize;

#pragma once

// The command-line mode: "anm2ed <info|frame|render> <file.anm2> [options]" looks at a file without the editor, as
// text or drawn (by the editor's OpenGL renderer, in a hidden window) to a PNG frame, a contact sheet, PNGs, a GIF or
// a video.
namespace anm2ed::cli
{
  bool is_cli(int, const char**);
  int run(int, const char**);
}

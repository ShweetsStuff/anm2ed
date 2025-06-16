#include "imgui.h"

void
imgui_init(SDL_Window* window, SDL_GLContext glContext)
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(window, glContext);
	ImGui_ImplOpenGL3_Init(STRING_OPENGL_VERSION);

	printf(STRING_INFO_IMGUI_INIT);
}

void
imgui_tick(State* state)
{
	ImGui_ImplSDL3_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Hello, world!");
	ImGui::Text("Color");
	ImGui::ColorEdit3("Color", value_ptr(state->clearColor));
	ImGui::End();
}

void
imgui_draw(void)
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void
imgui_free(void)
{
	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui::DestroyContext();

	printf(STRING_INFO_IMGUI_FREE);
}

#include <cmath>
#include <thread>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>

#include "ftxui/component/checkbox.hpp"
#include "ftxui/component/container.hpp"
#include "ftxui/component/input.hpp"
#include "ftxui/component/menu.hpp"
#include "ftxui/component/radiobox.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/toggle.hpp"
#include "ftxui/screen/string.hpp"
#include "ftxui/component/button.hpp"

#include <experimental/filesystem>
//#include <ros/package.h>

using namespace ftxui;
using namespace std;

struct Settings
{
	wstring world_wstring;
	vector<wstring> args_wstring;
	wstring fsm_wstring;
};

bool doRun = 0;
int shift = 0;

class SimComponent : public Component
{

	Container container = Container::Horizontal();

	RadioBox world_box;
	RadioBox fsm_box;

	Container args = Container::Vertical();
	CheckBox args_checkbox[6];

	Container buttoncontainer = Container::Horizontal();

	Button button_exit;
	Button button_run;

	Container subcontainer = Container::Vertical();
	Container input_container = Container::Horizontal();

	Input input_add;
	Menu input;
	Input executable;

public:
	~SimComponent() override {}
	SimComponent()
	{
		Add(&container);

		// Worlds ----------------------------------------------------------------
		//string path = ros::package::getPath("uuv_descriptions");

		// TODO: make it work on ROS packages
		vector<string> world_list;
		string worlds_path = "/home/benji/Projects/Vortex/manta_ws/src/Vortex-Simulator/uuv_descriptions/launch";
		for (const auto &entry : experimental::filesystem::directory_iterator(worlds_path))
		{
			world_list.push_back(entry.path().string());
			wstring tmp_wstring = entry.path().wstring();

			tmp_wstring = tmp_wstring.substr(tmp_wstring.rfind(L"/") + 1);
			tmp_wstring = tmp_wstring.substr(0, tmp_wstring.find(L"."));

			replace(tmp_wstring.begin(), tmp_wstring.end(), L'_', L' ');

			world_box.entries.push_back(tmp_wstring);
		}
		container.Add(&world_box);

		// Args    ----------------------------------------------------------------
		container.Add(&args);
		args_checkbox[0].label = L"-gui";
		args_checkbox[1].label = L"-camerafront";
		args_checkbox[2].label = L"-cameraunder";
		args_checkbox[3].label = L"-paused";
		args_checkbox[4].label = L"-set_timeout";
		args_checkbox[5].label = L"-timeout";

		for (auto &c : args_checkbox)
			args.Add(&c);

		// FSM ----------------------------------------------------------------
		vector<string> fsm_list;
		string fsm_path = "/home/benji/Projects/Vortex/manta_ws/src/Vortex-AUV/mission/finite_state_machine/launch";
		for (const auto &entry : experimental::filesystem::directory_iterator(fsm_path))
		{
			fsm_list.push_back(entry.path().string());
			wstring tmp_wstring = entry.path().wstring();

			tmp_wstring = tmp_wstring.substr(tmp_wstring.rfind(L"/") + 1);
			tmp_wstring = tmp_wstring.substr(0, tmp_wstring.find(L"."));

			replace(tmp_wstring.begin(), tmp_wstring.end(), L'_', L' ');

			fsm_box.entries.push_back(tmp_wstring);
		}
		container.Add(&fsm_box);

		container.Add(&subcontainer);
		// Executable
		// ----------------------------------------------------------------
		executable.placeholder = L"executable";
		subcontainer.Add(&executable);

		// Input    ----------------------------------------------------------------
		subcontainer.Add(&input_container);

		input_add.placeholder = L"input files";
		input_add.on_enter = [this] {
			input.entries.push_back(input_add.content);
			input_add.content = L"";
		};
		input_container.Add(&input_add);
		input_container.Add(&input);

		button_exit.label = L"Save & Exit";
		// button_exit.on_click = [&] { on_quit(); };
		button_exit.on_click = [&] { on_quit(); };
		buttoncontainer.Add(&button_exit);

		button_run.label = L"Run";
		button_run.on_click = [&] { doRun = 1; on_run(); };
		buttoncontainer.Add(&button_run);

		container.Add(&buttoncontainer);
	}

	Element Render() override
	{
		auto worlds_win = window(text(L"Worlds"), world_box.Render() | frame);
		auto args_win = window(text(L"Args"), args.Render());
		auto fsm_win = window(text(L"FSMs"), fsm_box.Render() | frame);
		auto button_exit_win = button_exit.Render();
		auto button_run_win = button_run.Render();
		auto executable_win = window(text(L"Executable:"), executable.Render());
		auto input_win =
			window(text(L"Input"),
				   hbox({
					   vbox({
						   hbox({
							   text(L"Add: "),
							   input_add.Render(),
						   }) | size(WIDTH, EQUAL, 20) |
							   size(HEIGHT, EQUAL, 1),
						   filler(),
					   }),
					   separator(),
					   input.Render() | frame | size(HEIGHT, EQUAL, 3) | flex,
				   }));
		return vbox({hbox({
						 worlds_win | size(HEIGHT, LESS_THAN, 6),
						 args_win,
						 fsm_win | size(HEIGHT, LESS_THAN, 6),
						 vbox({
							 executable_win | size(WIDTH, EQUAL, 20),
							 input_win | size(WIDTH, EQUAL, 40),
						 }),
						 filler(),
					 }),
					 hflow(RenderCommandLine_0()) | flex_grow,
					 hflow(RenderCommandLine_1()) | flex_grow,
					 hbox({
						 filler(),
						 button_exit_win | size(WIDTH, EQUAL, 15),
						 button_run_win | size(WIDTH, EQUAL, 10),
					 })}) |
			   flex_grow | border;
	}

	Elements RenderCommandLine_0()
	{
		Elements line_0;

		// Worlds
		line_0.push_back(text(L" "));
		line_0.push_back(text(world_box.entries[world_box.selected]) | bold);

		// Args
		for (auto &it : args_checkbox)
		{
			if (it.state)
			{
				line_0.push_back(text(L" "));
				line_0.push_back(text(it.label) | dim);
			}
		}

		// Executable
		if (!executable.content.empty())
		{
			line_0.push_back(text(L" -O ") | bold);
			line_0.push_back(text(executable.content) | color(Color::BlueLight) | bold);
		}
		// Input
		for (auto &it : input.entries)
		{
			line_0.push_back(text(L" " + it) | color(Color::RedLight));
		}
		return line_0;
	}

	Elements RenderCommandLine_1()
	{
		Elements line_1;

		// FSMs
		line_1.push_back(text(L" FSM: "));
		line_1.push_back(text(fsm_box.entries[fsm_box.selected]) | bold);
		return line_1;
	}

	Settings GetSettingsWString()
	{
		Settings sim_settings;
		wcout << world_box.entries[world_box.selected] << endl;
		wcout << fsm_box.entries[fsm_box.selected] << endl;

		sim_settings.world_wstring = world_box.entries[world_box.selected];
		sim_settings.fsm_wstring = fsm_box.entries[fsm_box.selected];

		for (auto &it : args_checkbox)
		{
			if (it.state)
			{
				sim_settings.args_wstring.push_back(it.label);
			}
		}

		return sim_settings;
	}

	function<void()> on_quit = [] {};
	function<void()> on_run = []() {};
};

class Tab : public Component
{
public:
	Container main_container = Container::Vertical();

	Toggle tab_selection;
	Container container = Container::Tab(&tab_selection.selected);

	SimComponent sim_component;

	Tab()
	{
		Add(&main_container);
		main_container.Add(&tab_selection);
		tab_selection.entries = {
			L"SIM",
			L"REAL",
			L"Options",
		};
		main_container.Add(&container);
		container.Add(&sim_component);
	}

	Element Render() override
	{
		return vbox({
			text(L"VortexNTNU") | bold | hcenter,
			tab_selection.Render() | hcenter,
			container.Render() | flex,
		});
	}
};

int main(int argc, const char *argv[])
{
	auto screen = ScreenInteractive::Fullscreen();

	std::thread update([&screen]() {
		for (;;)
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(0.05s);
			shift++;
			screen.PostEvent(Event::Custom);
		}
	});

	Tab tab;

	tab.sim_component.on_quit = screen.ExitLoopClosure();
	tab.sim_component.on_run = screen.ExitLoopClosure();

	screen.Loop(&tab);

	update.detach();

	if (doRun)
		Settings sim_settings = tab.sim_component.GetSettingsWString();
	return 0;
}

#include <cmath>
#include <thread>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iterator>

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

using namespace ftxui;
using namespace std;

bool 				doRun = 0;
bool				doExit = 0;
int					shift = 0;


struct SettingsAll
{
	wstring 		world_wstring;
	string 			world_string;
	int 			world_idx;

	vector<wstring> args_wstring;
	vector<int> 	args_idx;
	
	wstring 		fsm_wstring;
	string			fsm_string;
	int				fsm_idx;
};

struct Settings
{
	int 			world_idx;
	vector<int> 	args_idx;
	int				fsm_idx;
};

void saveConfig(Settings& config) {
    ofstream cFile ("../user_settings.txt");

	std::stringstream result;
	std::copy(config.args_idx.begin(), config.args_idx.end(), ostream_iterator<int>(result, " "));

  	cFile << "world_idx=" << config.world_idx <<  "\nargs_idx=" << result.str() << "\nfsm_idx=" << config.fsm_idx;

	//world_idx=3
	//args_idx=01234
	//fsm_idx=2
}

void loadConfig(Settings& config) {
    ifstream cFile ("../user_settings.txt");
    if (cFile.is_open())
    {
        string line;
        while(getline(cFile, line)){
            line.erase(remove_if(line.begin(), line.end(), ::isspace),
                                 line.end());
            if(line[0] == '#' || line.empty())
                continue;
            auto delimiterPos = line.find("=");
            auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);
            //cout << name << " " << value << " " << typeid(value).name() << '\n';

			if (name == "world_idx")
				config.world_idx = stoi(value);
			else if (name == "fsm_idx")
				config.fsm_idx = stoi(value);
			else if (name == "args_idx")
			{
				for (int i = 0; i < value.length(); i++)
					config.args_idx.push_back(value[i] - '0');
			}
		}
        
    }
    else {
        cerr << "Couldn't open config file for reading.\n";
    }
}

string exec(const char* cmd) {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

string get_rospack_path(const char* rospack_name) {
	string find_pkg(rospack_name);
	find_pkg = "rospack find " + find_pkg;
	string rospack_path = exec(find_pkg.c_str());
	return rospack_path.substr(0, rospack_path.size()-1);;
}

string get_roslaunch_path(const char* rospack_name) {
	return get_rospack_path(rospack_name) + "/launch";
}

class SimComponent : public Component
{

	Container container = Container::Horizontal();

	RadioBox world_box;
	RadioBox fsm_box;

	Container args = Container::Vertical();
	CheckBox args_checkbox[6];

	Container buttoncontainer = Container::Vertical();

	Button button_exit;
	Button button_run;

	Container subcontainer = Container::Vertical();
	Container input_container = Container::Horizontal();

	Input input_add;
	Menu input;
	Input executable;

public:
	vector<string> world_list_full_path;
	vector<string> fsm_list_full_path;

	vector<wstring> world_list;
	vector<wstring> fsm_list;

	~SimComponent() override {}
	SimComponent()
	{	
		Settings settings;
		loadConfig(settings);


		world_box.selected = settings.world_idx;
		Add(&container);

		// Buttons ----------------------------------------------------------------
		button_run.label = L"Run";
		button_run.on_click = [&] { doRun = 1; on_run(); };
		buttoncontainer.Add(&button_run);

		button_exit.label = L"Save & Exit";
		button_exit.on_click = [&] { doExit = 1; on_quit(); };
		buttoncontainer.Add(&button_exit);

		container.Add(&buttoncontainer);

		// Worlds ----------------------------------------------------------------
		//string worlds_path = "/home/benji/Projects/Vortex/manta_ws/src/Vortex-Simulator/simulator_launch/launch";
		
		string worlds_path = get_roslaunch_path("simulator_launch");
		
		for (const auto &entry : experimental::filesystem::directory_iterator(worlds_path))
		{
			world_list_full_path.push_back(entry.path().string());
			wstring tmp_wstring = entry.path().wstring();

			tmp_wstring = tmp_wstring.substr(tmp_wstring.rfind(L"/") + 1);
			world_list.push_back(tmp_wstring);
			tmp_wstring = tmp_wstring.substr(0, tmp_wstring.find(L"."));

			replace(tmp_wstring.begin(), tmp_wstring.end(), L'_', L' ');

			world_box.entries.push_back(tmp_wstring);
		}
		world_box.selected = settings.world_idx; 
		container.Add(&world_box);

		// Args    ----------------------------------------------------------------
		container.Add(&args);
		args_checkbox[0].label = L"gui";
		args_checkbox[1].label = L"camerafront";
		args_checkbox[2].label = L"cameraunder";
		args_checkbox[3].label = L"paused";
		args_checkbox[4].label = L"set_timeout";
		args_checkbox[5].label = L"timeout";

		for (auto &c : args_checkbox)
			args.Add(&c);

		//args_checkbox[0].state = true;

		for (auto &it : settings.args_idx)
		{
			args_checkbox[it].state = true;
		}

		// FSM ----------------------------------------------------------------
		//string fsm_path = "/home/benji/Projects/Vortex/manta_ws/src/Vortex-AUV/mission/finite_state_machine/launch";
		
		string fsm_path = get_roslaunch_path("finite_state_machine");
		for (const auto &entry : experimental::filesystem::directory_iterator(fsm_path))
		{
			fsm_list_full_path.push_back(entry.path().string());
			wstring tmp_wstring = entry.path().wstring();

			tmp_wstring = tmp_wstring.substr(tmp_wstring.rfind(L"/") + 1);
			fsm_list.push_back(tmp_wstring);
			tmp_wstring = tmp_wstring.substr(0, tmp_wstring.find(L"."));

			replace(tmp_wstring.begin(), tmp_wstring.end(), L'_', L' ');

			fsm_box.entries.push_back(tmp_wstring);
		}
		fsm_box.entries.push_back(L"none");
		fsm_list.push_back(L"none");
		fsm_box.selected = settings.fsm_idx; 
		container.Add(&fsm_box);
	}

	Element Render() override
	{
		auto worlds_win = window(text(L"Worlds"), world_box.Render() | frame);
		auto args_win = window(text(L"Args"), args.Render());
		auto fsm_win = window(text(L"FSMs"), fsm_box.Render() | frame);
		auto button_exit_win = button_exit.Render();
		auto button_run_win = button_run.Render();

		return vbox({
				   vbox({
					   text(L"VortexNTNU") | bold | hcenter,
					   hbox({
							text(L" Use <-- --> to navigate between windows") | dim,
						   filler(),
						   vbox({
							   button_run_win | size(WIDTH, EQUAL, 15),
							   button_exit_win | size(WIDTH, EQUAL, 15),
						   }),
					   }),
					   worlds_win |
						   size(HEIGHT, LESS_THAN, 10),
					   args_win,
					   fsm_win | size(HEIGHT, LESS_THAN, 10),
					   filler(),
				   }),
				   hflow(RenderCommandLine_0()) | flex_grow,
				   hflow(RenderCommandLine_1()) | flex_grow,
			   }) |
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

	SettingsAll GetSettingsAll(bool DEBUG = 0)
	{
		SettingsAll sim_settings;

		sim_settings.world_wstring = world_box.entries[world_box.selected];
		sim_settings.world_idx = world_box.selected;
		sim_settings.world_string = world_list_full_path[world_box.selected];

		sim_settings.fsm_wstring = fsm_box.entries[fsm_box.selected];
		sim_settings.fsm_idx = fsm_box.selected;
		sim_settings.fsm_string = fsm_list_full_path[fsm_box.selected];


		cout << "Arguments: " << endl;
		int count = 0;
		for (auto &it : args_checkbox)
		{
			if (it.state)
			{
				
				sim_settings.args_wstring.push_back(it.label);
				sim_settings.args_idx.push_back(count);

				wcout << it.label;
				cout << "   " << count << endl;
			}
			count++;
		}
		cout << endl;

		cout << "World: " << endl;
		wcout << world_box.entries[world_box.selected] << "   ";
		cout << sim_settings.world_string << "   ";
		cout << sim_settings.world_idx << endl << endl;

		cout << "FSM: " << endl;
		wcout << fsm_box.entries[fsm_box.selected] << "   ";
		cout << sim_settings.fsm_string << "   ";
		cout << sim_settings.fsm_idx << endl << endl;

		return sim_settings;
	}
	Settings GetSettings(bool DEBUG = 0)
	{
		Settings sim_settings;

		sim_settings.world_idx = world_box.selected;
		sim_settings.fsm_idx = fsm_box.selected;

		cout << "Arguments: " << endl;
		int count = 0;
		for (auto &it : args_checkbox)
		{
			if (it.state)
			{
				sim_settings.args_idx.push_back(count);

				cout << "   " << count << endl;
			}
			count++;
		}
		cout << endl;

		cout << "World: " << endl;
		cout << sim_settings.world_idx << endl << endl;

		cout << "FSM: " << endl;
		cout << sim_settings.fsm_idx << endl << endl;

		return sim_settings;
	}

	string GetArgsString() {
		string arguments = " ";
		for (auto &it : args_checkbox)
		{ 
			if (it.state)
			{
				string tmp_str(it.label.begin(), it.label.end());
				arguments += tmp_str;
				arguments += ":=1 ";
			}
			else {
				string tmp_str(it.label.begin(), it.label.end());
				arguments += tmp_str;
				arguments += ":=0 ";
			}
		}
		return arguments;
	}
	
	function<void()> on_quit = [] {};
	function<void()> on_run = []() {};
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

	SimComponent sim_component;

	sim_component.on_quit = screen.ExitLoopClosure();
	sim_component.on_run = screen.ExitLoopClosure();

	screen.Loop(&sim_component);

	update.detach();

	Settings sim_settings = sim_component.GetSettings();

	if (doExit) saveConfig(sim_settings);

	else if (doRun)
	{
		saveConfig(sim_settings);
		//int status = system("roslaunch simulator_launch cybernetics_sim.launch gui:=1 camerafront:=1 cameraunder:=0");

		string world_name(sim_component.world_list[sim_settings.world_idx].begin(), sim_component.world_list[sim_settings.world_idx].end());
		string fsm_name(sim_component.fsm_list[sim_settings.fsm_idx].begin(), sim_component.fsm_list[sim_settings.fsm_idx].end());

		string arguments = sim_component.GetArgsString();

		string command_0 = "( roslaunch simulator_launch ";
		command_0 += world_name;
		command_0 += arguments;
		command_0 += ") && fg";

		string command_1 = "( sleep 5 ; roslaunch auv_setup auv.launch type:=simulator) &";

		string command_2 = "( sleep 10 ; roslaunch finite_state_machine ";
		command_2 += fsm_name;
		command_2 += ") &";

		cout << "Commands to run the simulator setup: " << endl;
		cout << command_0 << endl << command_1 << endl << command_2 << endl << endl;


		if (fsm_name != "none")
			int status_0 = system(command_2.c_str());
		int status_1 = system(command_1.c_str());
		int status_2 = system(command_0.c_str());
		
		
		// int status_3 = system("( roslaunch simulator_launch cybernetics_launch.launch) && fg");
	}
	return 0;
}

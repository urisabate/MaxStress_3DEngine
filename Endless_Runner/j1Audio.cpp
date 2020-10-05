#include "p2Defs.h"
#include "p2Log.h"
#include "j1App.h"
#include "j1Audio.h"

#include "SDL/include/SDL.h"
#include "SDL_mixer\include\SDL_mixer.h"
#pragma comment( lib, "SDL_mixer/libx86/SDL2_mixer.lib" )

j1Audio::j1Audio() : j1Module()
{
	music = NULL;
	name.create("audio");

	folder_music = nullptr;
	folder_fx	 = nullptr;

	current_volume = SDL_MIX_MAXVOLUME;
}

// Destructor
j1Audio::~j1Audio()
{}

// Called before render is available
bool j1Audio::Awake(pugi::xml_node& config)
{
	//BROFILER_CATEGORY("Audio->Awake", Profiler::Color::HotPink)
	LOG("Loading Audio Mixer");
	bool ret = true;
	SDL_Init(0);

	if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		LOG("SDL_INIT_AUDIO could not initialize! SDL_Error: %s\n", SDL_GetError());
		active = false;
		ret = true;
	}

	// load support for the JPG and PNG image formats
	int flags = MIX_INIT_OGG;
	int init = Mix_Init(flags);

	if((init & flags) != flags)
	{
		LOG("Could not initialize Mixer lib. Mix_Init: %s", Mix_GetError());
		active = false;
		ret = true;
	}

	//Initialize SDL_mixer
	if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
	{
		LOG("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
		active = false;
		ret = true;
	}

	folder_music.create(config.child("music").child("folder").child_value());
	folder_fx.create(config.child("fx").child("folder").child_value());
	LOG("Folder music %s", folder_music.GetString());
	LOG("Folder fx %s", folder_fx.GetString());

	pugi::xml_node music_node = config.child("music").child("track");
	
	int i = 0;
	for (music_node; music_node; music_node = music_node.next_sibling("track")) {
		//p2SString music_path(music_node.child_value());
		tracks_path.add(music_node.child_value());

		LOG("Loading paths %s", tracks_path[i++].GetString());
	}
///////////////////////////////////
	pugi::xml_node fx_node = config.child("fx").child("sound");

	i = 0;
	for (fx_node; fx_node; fx_node = fx_node.next_sibling("sound")) {
		
		fx_path.add(fx_node.child_value());

		LOG("Loading fx path %s  i: %d", fx_path[i].GetString(),i-1);
		i++;
	}
///////////////////////////////////

	p2List_item<p2SString>* item = fx_path.start;

	while (item != nullptr)
	{
		LoadFx(PATH(folder_fx.GetString(), item->data.GetString()));
		item = item->next;
	}
	return ret;
}

// Called before quitting
bool j1Audio::CleanUp()
{
	BROFILER_CATEGORY("Audio->CleanUp", Profiler::Color::HotPink)
	if(!active)
		return true;

	LOG("Freeing sound FX, closing Mixer and Audio subsystem");

	if(music != NULL)
	{
		Mix_FreeMusic(music);
	}

	p2List_item<Mix_Chunk*>* item;
	for (item = fx.start; item != NULL; item = item->next) {
		Mix_FreeChunk(item->data);
		//RELEASE(item->data);
	}

	item->~p2List_item();
	fx.clear();

	Mix_CloseAudio();
	Mix_Quit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	return true;
}

// Play a music file
bool j1Audio::PlayMusic(const char* path, float fade_time)
{
	bool ret = true;

	if(!active)
		return false;

	if(music != NULL)
	{
		if(fade_time > 0.0f)
		{
			Mix_FadeOutMusic(int(fade_time * 1000.0f));
		}
		else
		{
			Mix_HaltMusic();
		}

		// this call blocks until fade out is done
		Mix_FreeMusic(music);
	}

	music = Mix_LoadMUS(path);

	if(music == NULL)
	{
		LOG("Cannot load music %s. Mix_GetError(): %s\n", path, Mix_GetError());
		ret = false;
	}
	else
	{
		if(fade_time > 0.0f)
		{
			if(Mix_FadeInMusic(music, -1, (int) (fade_time * 1000.0f)) < 0)
			{
				LOG("Cannot fade in music %s. Mix_GetError(): %s", path, Mix_GetError());
				ret = false;
			}
		}
		else
		{
			if(Mix_PlayMusic(music, -1) < 0)
			{
				LOG("Cannot play in music %s. Mix_GetError(): %s", path, Mix_GetError());
				ret = false;
			}
		}
	}

	LOG("Successfully playing %s", path);
	return ret;
}

// Load WAV
unsigned int j1Audio::LoadFx(const char* path)
{
	unsigned int ret = 0;

	if(!active)
		return 0;

	Mix_Chunk* chunk = Mix_LoadWAV(path);

	if(chunk == NULL)
	{
		LOG("Cannot load wav %s. Mix_GetError(): %s", path, Mix_GetError());
	}
	else
	{
		fx.add(chunk);
		ret = fx.count();
	}
	//RELEASE(chunk);
	return ret;
}

// Play WAV
bool j1Audio::PlayFx(unsigned int id, int repeat)
{
	bool ret = false;

	if(!active)
		return false;

	if(id > 0 && id <= fx.count())
	{
		Mix_PlayChannel(-1, fx[id - 1], repeat);
	}

	return ret;
}

void j1Audio::ToggleMusicPause()
{
	if (Mix_PausedMusic() > 0)
	{
		Mix_ResumeMusic();
	}
	else
	{
		Mix_PauseMusic();
	}
}


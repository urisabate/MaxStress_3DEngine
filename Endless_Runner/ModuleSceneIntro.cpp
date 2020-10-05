#include "Globals.h"
#include "Application.h"
#include "ModuleSceneIntro.h"
#include "Primitive.h"
#include "PhysBody3D.h"
#include "PhysVehicle3D.h"

#define SIZE_ARRAY(_A_) (sizeof(_A_)/sizeof(_A_[0]))

// Circuit Def
struct CubeDef
{
	float size_x, size_y, size_z;
	float pos_x, pos_y, pos_z;
	Color _color;
	bool has_respawn = true;
	float _mass = 0;
	bool _rotate = false;
	float _angle = 0;
	const vec3 _axis = { 0, 0, 0 };
	float sensor_rot = 0;
	const vec3 sensor_axis = { 0,0,0 };
	bool add_collision_listener = false;
};

CubeDef cube_defs[] =
{
	{ 25,  3, 15, 0, 1, -20, White },
	{ 25, 3, 5, 0, 1, -10, Black, false },
	{ 25,  3, 75, 0, 1, 30, White },
	{ 12, 1, 100, 0, 0.5, 50, White, false, 0, true, -20,{ 1, 0, 0 } },
	{ 25, 1, 40, 0, 17.6, 116.18, White },
	{ 25, 1, 55, 15.8, 17.55, 147.5, White, true, 0, true, 45,{ 0, 1, 0 }, 45,{ 0, 1, 0 } },
	{ 25, 1, 90, 71.6, 17.6, 163.4, White, true, 0, true, 90,{ 0, 1, 0 }, 90 ,{ 0, 1, 0 } },
	{ 25, 1, 90, 108, 17.6, 131, White, true, 0, false, 0,{ 0, 1, 0 }, 180,{ 0, 1, 0 } },
	{ 25, 1, 40, 108, 17.6, 100, White, false, 0, true, 20,{ 1, 0, 0 }, 180,{ 0, 1, 0 } },
	{ 25, 1, 90, 108, 5.5, 0, White, true, 0, false, 0,{ 0, 0, 0 }, 180,{ 0, 1, 0 } },
	{ 120, 3, 25, 47, 1, -32, White, true, 0, false, 0,{ 0, 0, 0 }, 270,{ 0, 1, 0 } }
};

ModuleSceneIntro::ModuleSceneIntro(Application* app, bool start_enabled) : Module(app, start_enabled)
{

}

ModuleSceneIntro::~ModuleSceneIntro()
{}

// Load assets
bool ModuleSceneIntro::Start()
{
	LOG("Loading Intro assets");
	bool ret = true;

	for (int i = 0; i < SIZE_ARRAY(cube_defs); i++)
	{
		Cube c;
		c.size.Set(cube_defs[i].size_x, cube_defs[i].size_y, cube_defs[i].size_z);
		c.SetPos(cube_defs[i].pos_x, cube_defs[i].pos_y, cube_defs[i].pos_z);

		c.color = cube_defs[i]._color;

		if (cube_defs[i]._rotate)
			c.SetRotation(cube_defs[i]._angle, cube_defs[i]._axis);

		if (cube_defs[i].has_respawn == true)
			CreateSensor(cube_defs[i].pos_x, cube_defs[i].pos_y + 5, cube_defs[i].pos_z, 30, 20, 0.1f, Respawn, cube_defs[i].sensor_rot, cube_defs[i].sensor_axis);

		PhysBody3D *p = App->physics->AddBody(c, cube_defs[i]._mass);
		if (cube_defs[i].add_collision_listener)
			p->collision_listeners.add(this);
		cubes.add(c);
	}

	CreateSensor(0, 0, -8, 30, 20, 0.1f, LapSensor);
	CreateSensor(110, 20, 100, 30, 20, 0.1f, HalfLap);

	current_track = App->audio->tracks_path.getFirst();
	App->audio->PlayMusic(current_track->data.GetString());

	return ret;
}

// Load assets
bool ModuleSceneIntro::CleanUp()
{
	LOG("Unloading Intro scene");

	return true;
}

// Update
update_status ModuleSceneIntro::Update(float dt)
{
	if ((App->player1->win == true || App->player2->win == true) && first_iteration)
	{
		current_track = App->audio->tracks_path.getLast();
		App->audio->PlayMusic(current_track->data.GetString());

		first_iteration = false;
	}

	if (App->player1->win == true)
	{
		char title[80];
		sprintf_s(title, "PLAYER 1 WINS    *Press Space to Start a New Game*");
		App->window->SetTitle(title);
	}
	else if (App->player2->win == true)
	{
		char title[80];
		sprintf_s(title, "PLAYER 2 WINS    *Press Space to Start a New Game*");
		App->window->SetTitle(title);
	}
	else
	{
		char title[80];
		sprintf_s(title, "Player 1: %d/3 || Player 2: %d/3", App->player1->laps, App->player2->laps);
		App->window->SetTitle(title);
	}

	if (App->player1->respawn_num > App->player2->respawn_num && App->player1->laps >= App->player2->laps)
	{
		App->player1->first = true;
		App->player2->first = false;
	}
	if (App->player2->respawn_num > App->player1->respawn_num && App->player2->laps >= App->player1->laps)
	{
		App->player1->first = false;
		App->player2->first = true;
	}

	if (abs(App->player1->vehicle->GetPos().getZ() - App->player2->vehicle->GetPos().getZ()) < 1
		&& abs(App->player1->vehicle->GetPos().getX() - App->player2->vehicle->GetPos().getX()) < 30)
	{
		if (App->player1->vehicle->GetKmh() > App->player2->vehicle->GetKmh()
			&& App->player1->laps >= App->player2->laps)
		{
			App->player1->first = true;
			App->player2->first = false;
		}
		else if (App->player2->vehicle->GetKmh() > App->player1->vehicle->GetKmh()
			&& App->player2->laps >= App->player1->laps)
		{
			App->player1->first = false;
			App->player2->first = true;
		}
	}

	return UPDATE_CONTINUE;
}

void ModuleSceneIntro::OnCollision(PhysBody3D* body1, PhysBody3D* body2)
{
	if (body1->type == HalfLap)
	{
		if (body2->type == Car1 && App->player1->half_lap == false)
		{
			App->player1->half_lap = true;
		}
		else if (body2->type == Car2 && App->player2->half_lap == false)
		{
			App->player2->half_lap = true;
		}
	}

	else if (body1->type == LapSensor)
	{
		if (body2->type == Car1)
		{
			if (App->player1->laps < 3 && App->player1->half_lap == true)
			{
				App->player1->laps++;
				App->player1->half_lap = false;
			}
			else if (App->player1->laps == 3 && App->player1->half_lap == true)
			{
				App->player1->win = true;
			}
		}
		else if (body2->type == Car2)
		{
			if (App->player2->laps < 3 && App->player2->half_lap == true)
			{
				App->player2->laps++;
				App->player2->half_lap = false;
			}
			else if (App->player2->laps == 3 && App->player2->half_lap == true)
			{
				App->player2->win = true;
			}
		}
	}

	else if (body1->type == Respawn)
	{
		p2List_item<PhysBody3D*>* item = respawns.getFirst();

		for (int i = 0; item != nullptr; item = item->next)
		{
			if (body2->type == Car1)
			{
				if (body1->GetPos() == item->data->GetPos())
				{
					App->player1->respawn_pos = item->data->GetPos();
					App->player1->respawn_rot = item->data->GetRotation();
					App->player1->respawn_num = i;
				}
			}
			if (body2->type == Car2)
			{
				if (body1->GetPos() == item->data->GetPos())
				{
					App->player2->respawn_pos = item->data->GetPos();
					App->player2->respawn_rot = item->data->GetRotation();
					App->player2->respawn_num = i;
				}
			}
			i++;
		}
	}
}

void ModuleSceneIntro::CreateSensor(float x, float y, float z, float i, float j, float k, SensorType type, float rot, vec3 axis)
{
	Cube ret(i, j, k);
	ret.SetPos(x, y, z);

	if (rot != 0.0f)
		ret.SetRotation(rot, axis);

	PhysBody3D* pbody = App->physics->AddBody(ret, 0, type);
	pbody->SetSensor();
	pbody->collision_listeners.add(this);

	if (type == Respawn)
	{
		respawns.add(pbody);
	}
}
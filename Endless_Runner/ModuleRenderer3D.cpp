#include "Globals.h"
#include "Application.h"
#include "ModuleRenderer3D.h"
#include "SDL\include\SDL_opengl.h"

#include "ModulePlayer1.h"
#include "ModulePlayer2.h"
#include "PhysVehicle3D.h"
#include "ModulePhysics3D.h"
#include "ModuleSceneIntro.h"

#include <gl/GL.h>
#include <gl/GLU.h>

#pragma comment (lib, "glu32.lib")    /* link OpenGL Utility lib     */
#pragma comment (lib, "opengl32.lib") /* link Microsoft OpenGL lib   */

ModuleRenderer3D::ModuleRenderer3D(Application* app, bool start_enabled) : Module(app, start_enabled)
{
}

// Destructor
ModuleRenderer3D::~ModuleRenderer3D()
{}

// Called before render is available
bool ModuleRenderer3D::Init()
{
	LOG("Creating 3D Renderer context");
	bool ret = true;
	
	//Create context
	context = SDL_GL_CreateContext(App->window->window);
	if(context == NULL)
	{
		LOG("OpenGL context could not be created! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}
	
	if(ret == true)
	{
		//Use Vsync
		if(VSYNC && SDL_GL_SetSwapInterval(1) < 0)
			LOG("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());

		//Initialize Projection Matrix
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		//Check for error
		GLenum error = glGetError();
		if(error != GL_NO_ERROR)
		{
			LOG("Error initializing OpenGL! %s\n", gluErrorString(error));
			ret = false;
		}

		//Initialize Modelview Matrix
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		//Check for error
		error = glGetError();
		if(error != GL_NO_ERROR)
		{
			LOG("Error initializing OpenGL! %s\n", gluErrorString(error));
			ret = false;
		}
		
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		glClearDepth(1.0f);
		
		//Initialize clear color
		glClearColor(0.f, 0.f, 0.f, 1.f);

		//Check for error
		error = glGetError();
		if(error != GL_NO_ERROR)
		{
			LOG("Error initializing OpenGL! %s\n", gluErrorString(error));
			ret = false;
		}
		
		GLfloat LightModelAmbient[] = {0.0f, 0.0f, 0.0f, 1.0f};
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, LightModelAmbient);
		
		lights[0].ref = GL_LIGHT0;
		lights[0].ambient.Set(0.25f, 0.25f, 0.25f, 1.0f);
		lights[0].diffuse.Set(0.75f, 0.75f, 0.75f, 1.0f);
		lights[0].SetPos(0.0f, 0.0f, 2.5f);
		lights[0].Init();
		
		GLfloat MaterialAmbient[] = {1.0f, 1.0f, 1.0f, 1.0f};
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, MaterialAmbient);

		GLfloat MaterialDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialDiffuse);
		
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		lights[0].Active(true);
		glEnable(GL_LIGHTING);
		glEnable(GL_COLOR_MATERIAL);
	}

	// Projection matrix for
	OnResize(SCREEN_WIDTH, SCREEN_HEIGHT);

	return ret;
}

// PreUpdate: clear buffer
update_status ModuleRenderer3D::PreUpdate(float dt)
{
	Plane p(0, 1, 0, 0);
	p.axis = true;

	glClear(GL_COLOR_BUFFER_BIT |  GL_DEPTH_BUFFER_BIT);

	//--------------------- top viewport ------------------------
	glViewport(0, App->window->height / 2, App->window->width, App->window->height / 2);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	ProjectionMatrix = perspective(60.0f, (float)App->window->width / (App->window->height * 0.5f), 0.125f, 512.0f);
	glLoadMatrixf(&ProjectionMatrix);
	glMatrixMode(GL_MODELVIEW);

	//position camera on player 1
	App->camera->Position.x = App->player1->vehicle->vehicle->getChassisWorldTransform().getOrigin().getX() - 10 * App->player1->vehicle->vehicle->getForwardVector().getX();
	App->camera->Position.y = App->player1->vehicle->vehicle->getChassisWorldTransform().getOrigin().getY() + 10.5f * App->player1->vehicle->vehicle->getUpAxis();
	App->camera->Position.z = App->player1->vehicle->vehicle->getChassisWorldTransform().getOrigin().getZ() - 12.5f * App->player1->vehicle->vehicle->getForwardVector().getZ();
	float x = App->player1->vehicle->vehicle->getChassisWorldTransform().getOrigin().getX() + 30 * App->player1->vehicle->vehicle->getForwardVector().getX();
	float z = App->player1->vehicle->vehicle->getChassisWorldTransform().getOrigin().getZ() + 30 * App->player1->vehicle->vehicle->getForwardVector().getZ();
	App->camera->LookAt(vec3(x, 1, z));
	App->camera->CalculateViewMatrix();
	glLoadMatrixf(App->camera->GetViewMatrix());

	// light 0 on cam pos
	lights[0].SetPos(App->camera->Position.x, App->camera->Position.y, App->camera->Position.z);

	if (App->player1->first == true) //plane color and render
	{
		p.color = Green;
	}
	else
	{
		p.color = Red;
	}
	p.Render();

	for (uint i = 0; i < MAX_LIGHTS; ++i) //lights render
		lights[i].Render();

	p2List_item<PhysVehicle3D*>* item = App->physics->vehicles.getFirst(); 	// Render vehicles
	while (item)
	{
		item->data->Render();
		item = item->next;
	}

	p2List_item<Cube>* item2 = App->scene_intro->cubes.getFirst(); // Render map
	while (item2)
	{
		item2->data.Render();
		item2 = item2->next;
	}

	if (App->physics->debug == true) //debug draw
	{
		App->physics->world->debugDrawWorld();
	}

	//--------------------- bottom viewport ------------------------
	glViewport(0, 0, App->window->width, App->window->height / 2);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	ProjectionMatrix = perspective(60.0f, (float)App->window->width / (App->window->height * 0.5f), 0.125f, 512.0f);
	glLoadMatrixf(&ProjectionMatrix);
	glMatrixMode(GL_MODELVIEW);

	//position camera on player 2
	App->camera->Position.x = App->player2->vehicle->vehicle->getChassisWorldTransform().getOrigin().getX() - 10 * App->player2->vehicle->vehicle->getForwardVector().getX();
	App->camera->Position.y = App->player2->vehicle->vehicle->getChassisWorldTransform().getOrigin().getY() + 10.5f * App->player2->vehicle->vehicle->getUpAxis();
	App->camera->Position.z = App->player2->vehicle->vehicle->getChassisWorldTransform().getOrigin().getZ() - 12.5f * App->player2->vehicle->vehicle->getForwardVector().getZ();
	float x2 = App->player2->vehicle->vehicle->getChassisWorldTransform().getOrigin().getX() + 30 * App->player2->vehicle->vehicle->getForwardVector().getX();
	float z2 = App->player2->vehicle->vehicle->getChassisWorldTransform().getOrigin().getZ() + 30 * App->player2->vehicle->vehicle->getForwardVector().getZ();
	App->camera->LookAt(vec3(x2, 1, z2));
	App->camera->CalculateViewMatrix();
	glLoadMatrixf(App->camera->GetViewMatrix());

	// light 0 on cam pos
	lights[0].SetPos(App->camera->Position.x, App->camera->Position.y, App->camera->Position.z);

	if (App->player2->first == true) //plane color and render
	{
		p.color = Green;
	}
	else
	{
		p.color = Red;
	}
	p.Render();

	for (uint i = 0; i < MAX_LIGHTS; ++i) //lights render
		lights[i].Render();

	item = App->physics->vehicles.getFirst(); 	// Render vehicles
	while (item)
	{
		item->data->Render();
		item = item->next;
	}

	item2 = App->scene_intro->cubes.getFirst(); // Render map
	while (item2)
	{
		item2->data.Render();
		item2 = item2->next;
	}

	if (App->physics->debug == true) //debug draw
	{
		App->physics->world->debugDrawWorld();
	}

	//---
	return UPDATE_CONTINUE;
}

// PostUpdate present buffer to screen
update_status ModuleRenderer3D::PostUpdate(float dt)
{
	SDL_GL_SwapWindow(App->window->window);

	return UPDATE_CONTINUE;
}

// Called before quitting
bool ModuleRenderer3D::CleanUp()
{
	LOG("Destroying 3D Renderer");

	SDL_GL_DeleteContext(context);

	return true;
}


void ModuleRenderer3D::OnResize(int width, int height)
{
	glViewport(0, 0, width, height);

	App->window->width = width;
	App->window->height = height;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	ProjectionMatrix = perspective(60.0f, (float)width / (height * 0.5f) , 0.125f, 512.0f);
	glLoadMatrixf(&ProjectionMatrix);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        //define something for Windows (64-bit only)
        #include "../headers/Renderer.h"
        #include "../headers/GeometryNode.h"
        #include "../headers/Tools.h"
        #include <algorithm>
        #include "../headers/ShaderProgram.h"
        #include "glm/gtc/type_ptr.hpp"
        #include "glm/gtc/matrix_transform.hpp"
        #include "../headers/OBJLoader.h"
        #include <SDL2/SDL.h>
        #include <iostream>
        #include <fstream>
		#include <cstdlib>
		#include <ctime>
    #endif
#elif __APPLE__
    // apple
    #include "../headers/Renderer.h"
    #include "../headers/GeometryNode.h"
    #include "../headers/Tools.h"
    #include <algorithm>
    #include "../headers/ShaderProgram.h"
    #include "../glm/gtc/type_ptr.hpp"
    #include "../glm/gtc/matrix_transform.hpp"
    #include "../headers/OBJLoader.h"
    #include <iostream>
    #include <fstream>
	#include <cstdlib>
	#include <ctime>
#elif __linux__
    // linux
    #include "../headers/Renderer.h"
    #include "../headers/GeometryNode.h"
    #include "../headers/Tools.h"
    #include <algorithm>
    #include "../headers/ShaderProgram.h"
    #include "../glm/gtc/type_ptr.hpp"
    #include "../glm/gtc/matrix_transform.hpp"
    #include "../headers/OBJLoader.h"
    #include <iostream>
    #include <fstream>
	#include <cstdlib>
	#include <ctime>
#endif

#define TERRAIN_WIDTH 20
#define TERRAIN_HEIGHT 20

// RENDERER
Renderer::Renderer()
{
	m_first_frame = true;
	m_playing_defeat = false;

	m_blue_tower_counter = 1;
	m_red_tower_counter = 1;

	m_blue_tower_replace_counter = 0;
	m_red_tower_replace_counter = 0;

	m_lose_coins_effect = 0;
	m_lose_coins_effect_frame = 0;
	m_lose_coins_effect_duration = 15;	// duration in frames, better odd numbers

	m_skeleton_counter = 3;

	GAME_OVER = false;
	m_vbo_fbo_vertices = 0;
	m_vao_fbo = 0;

	m_terrain = nullptr;
	m_chest_object = nullptr;

	m_fbo = 0;
	m_fbo_texture = 0;

	m_rendering_mode = RENDERING_MODE::TRIANGLES;
	m_continuous_time = 0.0;

	// initialize the camera parameters
	m_camera_position = glm::vec3(1, 12, -7);
	m_camera_position_temp = m_camera_position;
	//m_camera_position = glm::vec3(0.720552, 18.1377, -11.3135);

	m_camera_target_position = glm::vec3(6, 0, 4);
	//m_camera_target_position = glm::vec3(4.005, 12.634, -5.66336);
	m_camera_up_vector = glm::vec3(0, 1, 0);

	m_blue_timer = 0.f;
	m_red_timer = 0.f;

	m_skeletons_wave_timer = 0.0f;
	m_level = 0;
	m_particles_timer = 0;
	hit = false;
	exploded_cannonball_index = 0;
	m_place_new_tower_time_limit = 20;
	Mix_AllocateChannels(16);
	m_default_tower = true;
	m_path_has_changed = false;
	MichaelJacksonMode = false;
	MichaelJacksonCounter = 0;
	themeCounter = 0;
	m_highscore = 0.f;

}

Renderer::~Renderer()
{
	// delete g_buffer
	glDeleteTextures(1, &m_fbo_texture);
	glDeleteFramebuffers(1, &m_fbo);

	// delete common data
	glDeleteVertexArrays(1, &m_vao_fbo);
	glDeleteBuffers(1, &m_vbo_fbo_vertices);

    delete m_terrain;
    delete m_chest_object;
    delete m_tower_object;
    delete m_road_object;
    delete m_green_tile;
    for (int i=0; i< 6; i++)
        delete m_skeleton_object[i];
    delete m_cannonball_object;
    delete m_menu_object;
    delete m_tower2_object;
    delete m_rocket_object;
}

bool Renderer::Init(int SCREEN_WIDTH, int SCREEN_HEIGHT)
{
	this->m_screen_width = SCREEN_WIDTH;
	this->m_screen_height = SCREEN_HEIGHT;

	// Initialize OpenGL functions

	//Set clear color
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	//This enables depth and stencil testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearDepth(1.0f);
	// glClearDepth sets the value the depth buffer is set at, when we clear it

	// Enable back face culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// open the viewport
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); //we set up our viewport

	bool techniques_initialization = InitRenderingTechniques();
	bool buffers_initialization = InitDeferredShaderBuffers();
	bool items_initialization = InitCommonItems();
	bool lights_sources_initialization = InitLightSources();
	bool meshes_initialization = InitGeometricMeshes();

	// Set the starting player position to (0, 0.02, 0)
    m_player_tile_position.x = 0.f;
    m_player_tile_position.y = 0.02f;
    m_player_tile_position.z = 0.f;

	// Set random seed
	srand(time(NULL));

	//If there was any errors
	if (Tools::CheckGLError() != GL_NO_ERROR)
	{
		printf("Exiting with error at Renderer::Init\n");
		return false;
	}
	//If everything initialized
	return techniques_initialization && items_initialization && buffers_initialization && meshes_initialization && lights_sources_initialization;
}

void Renderer::Update(float dt)
{

	if (m_first_frame)
	{
		Audio::PlayMusic("Theme.wav");
		m_first_frame = false;
	}

	if (GAME_OVER && !m_playing_defeat)
	{
		Audio::PlayMusic("Defeat.wav");
		m_playing_defeat = true;
	}

	float movement_speed = 4.0f;

	if (GAME_OVER)
	{
		movement_speed /= 2;
	}

	// compute the direction of the camera
	glm::vec3 direction = glm::normalize(m_camera_target_position - m_camera_position);

    // move the camera towards the direction of the camera
    m_camera_position_temp += m_camera_movement.x *  movement_speed * direction * dt;
    if(m_camera_position_temp.y >= 0.1 )
    {
        m_camera_position += m_camera_movement.x *  movement_speed * direction * dt;
        m_camera_target_position += m_camera_movement.x * movement_speed * direction * dt;
    }else if(m_camera_target_position.y > -7.5f)
    {
        m_camera_position.y = 0.1;
        m_camera_position += m_camera_movement.x *  movement_speed * direction * dt;
        m_camera_target_position += m_camera_movement.x * movement_speed * direction * dt;
    }

    // move the camera sideways
	glm::vec3 right = glm::normalize(glm::cross(direction, m_camera_up_vector));
	m_camera_position += m_camera_movement.y *  movement_speed * right * dt;
	m_camera_target_position += m_camera_movement.y * movement_speed * right * dt;

	glm::mat4 rotation = glm::mat4(1.0f);
	float angular_speed = glm::pi<float>() * 0.0025f;
	
	// compute the rotation of the camera based on the mouse movement
	rotation *= glm::rotate(glm::mat4(1.0), m_camera_look_angle_destination.y * angular_speed, right);
	rotation *= glm::rotate(glm::mat4(1.0), -m_camera_look_angle_destination.x  * angular_speed, m_camera_up_vector);
	m_camera_look_angle_destination = glm::vec2(0);
	
	// rotate the camera direction
	direction = rotation * glm::vec4(direction, 0);
	float dist = glm::distance(m_camera_position, m_camera_target_position);
    m_camera_target_position = m_camera_position + direction * 2.f; //anti gia dist 2

	// compute the view matrix
    m_view_matrix = glm::lookAt(m_camera_position, m_camera_target_position, m_camera_up_vector);

    m_terrain_transformation_matrix =
		glm::translate(glm::mat4(1.f), glm::vec3(9, 0, 9)) *
		glm::scale(glm::mat4(1.f), glm::vec3(10.f));
    m_terrain_transformation_normal_matrix = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_terrain_transformation_matrix))));

    glm::vec3 chest_position = glm::vec3(12, 0, 0);
    chest->setPosition(chest_position);

    //menu->SetPosition(rotation*glm::vec4(m_camera_position + glm::vec3(0,0,1),1));//set menu

    m_road_object_transformation_matrix =
		glm::translate(glm::mat4(1.0), glm::vec3(2, 0.01, 0));
    m_road_object_transformation_normal_matrix = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_road_object_transformation_matrix))));

    m_player_tile_transformation_matrix =
        glm::translate(glm::mat4(1.0), m_player_tile_position);
    m_player_tile_transformation_normal_matrix = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_player_tile_transformation_matrix))));

	MoveSkeleton(dt);
    //Michael Jackson Mode
    if(MichaelJacksonMode)
    {
        if(MichaelJacksonCounter==0)
        {
            Audio::PlayMusic("MJ.wav");
            MichaelJacksonCounter++;
            themeCounter=0;
        }
        for (size_t i = 0; i < m_skeletons.size(); i++)
        {
            if(m_skeletons[i].get_health() > 0)
                m_skeletons[i].setMichaelJacksonMode(true);
        }
    }else
    {
        if(themeCounter==0)
        {
            Audio::PlayMusic("theme.wav");
            MichaelJacksonCounter=0;
            themeCounter++;
        }
        for (size_t i = 0; i < m_skeletons.size(); i++)
        {
            if(m_skeletons[i].get_health() > 0)
                m_skeletons[i].setMichaelJacksonMode(false);
        }
    }


	if (isValidTowerPos())
	{
		m_player_tile = m_green_tile;
	}
	else
    {
        m_player_tile = m_red_tile;
    }

	m_continuous_time += dt;

    shoot(dt);

	m_blue_timer += dt;
	m_red_timer += dt;

	// every X seconds increase available blue towers
    if (m_blue_timer > 30.f && m_blue_tower_counter <= 5)
	{
		m_blue_tower_counter++;
		m_blue_timer = 0.f;
	}

	// every X seconds increase available red towers
    if (m_red_timer > 1.f * 60.f && m_red_tower_counter <= 5)
	{
		m_red_tower_counter++;
		m_red_timer = 0.f;
	}

    // show dead skeletons as dead
    for (size_t i = 0; i < m_skeletons.size(); i++)
    {
        if (m_skeletons[i].get_health() == 0)
        {
            m_skeletons[i].kill();
        }
    }

    //Do not render the skeletons that reached the tresure(always the leader)
    if(chest->isReached(m_skeletons))
    {
		m_lose_coins_effect = 1;
        for (size_t i = 0; i < m_skeletons.size(); i++)
        {
            if (m_skeletons[i].get_health() == -1)
            {
                m_skeletons[i].render(false);
            }
        }
    }

    //End the game if the chest is empty
    if (chest->getCoinsLeft() == 0 && !GAME_OVER)
	{
		GAME_OVER = true;
        std::cout<<"Highscore = " << m_highscore<< " seconds" <<std::endl;
    }else
    {
        m_highscore += dt;
    }

    //throw cannonballs
    m_particles_timer += dt;
    for (size_t i = 0; i < m_cannonballs.size();)
    {
        if (!m_cannonballs[i].update(dt, m_skeletons, 3.0f))
        {
            hit = true;
            m_explosion_position = m_cannonballs[i].getPosition();
            if(!(m_explosion_position.x > 19.5 || m_explosion_position.x < -1.0f || m_explosion_position.z > 19.5 || m_explosion_position.z < -1.0f))//when ball reached the end of the board
            {
                m_particle_emitters.emplace_back(m_explosion_position);
                m_particle_emitters[m_particle_emitters.size()-1].Init();
                m_particles_timer = 0;
            }
            m_cannonballs.erase(m_cannonballs.begin() + i);
        }
        else
        {
            i++;
        }
    }

    //throw rockets
    m_particles_timer += dt;
    for (size_t i = 0; i < m_rockets.size();)
    {
		if (m_skeletons[m_rockets[i].get_target()].get_health() <= 0)
		{
			m_rockets.erase(m_rockets.begin() + i);
			continue;
		}
        if (!m_rockets[i].update(dt, m_skeletons))
        {
            hit = true;
            m_explosion_position = m_rockets[i].getPosition();
            m_particle_emitters.emplace_back(m_explosion_position);
            m_particle_emitters[m_particle_emitters.size()-1].Init();
            m_particles_timer = 0;
            m_rockets.erase(m_rockets.begin() + i);
        }
        else
        {
            i++;
        }
    }
	
	for (int i = 0; i < m_particle_emitters.size(); i++)
	{
		m_particle_emitters[i].Update(dt);
	}

	for (int i = 0; i < m_particle_emitters.size(); i++)
	{
		if (m_particle_emitters[i].get_continuous_time() > 0.2) //delete particles after 0.2 seconds
		{
			m_particle_emitters.erase(m_particle_emitters.begin() + i);
		}
	}

    // place new and more powerfull skeletons every 20 secods
    //first find the last alive skeleton to prevent new waves from overlapping him
    m_last_alive_skeleton.set_health(0);
    for(int i=m_skeletons.size()-1; i>-1; i--)
    {
        if(m_skeletons[i].get_health()>0)
        {
            m_last_alive_skeleton = m_skeletons[i];
            break;
        }
    }
    bool wave_clear = true;
    for(int i=0; i<m_skeletons.size(); i++)
    {
        if(m_skeletons[i].get_health()>0)
        {
            wave_clear = false;
            break;
        }
    }
    if(wave_clear)
    {
         m_skeletons_wave_timer += dt;
    }

    //Now generate the new wave
	if (m_skeletons_wave_timer >= 3 && !GAME_OVER)
	{
		int j = 0;
		while (m_skeletons.size() > 0)
		{
			m_skeletons.erase(m_skeletons.begin());
		}
		m_skeletons_wave_timer = 0;
        m_level++;
        SpawnNewSkeletons();
	}

    //Towers are removed in style!
    for(int i = 0; i<m_towers.size(); i++)
    {
        if(m_towers[i].to_be_removed())
        {
            m_towers[i].Remove(dt);
            if(m_towers[i].getPosition().y < -3)
            {
                m_towers.erase(m_towers.begin()+i);
            }
        }

    }
}

bool Renderer::InitCommonItems()
{
	glGenVertexArrays(1, &m_vao_fbo);
	glBindVertexArray(m_vao_fbo);

	GLfloat fbo_vertices[] = {
		-1, -1,
		1, -1,
		-1, 1,
		1, 1,
	};

	glGenBuffers(1, &m_vbo_fbo_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_fbo_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(fbo_vertices), fbo_vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindVertexArray(0);

	return true;
}

bool Renderer::InitRenderingTechniques()
{
    bool initialized = true;

    // Geometry Rendering Program
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        std::string vertex_shader_path = "../Data/Shaders/basic_rendering.vert";
        std::string fragment_shader_path = "../Data/Shaders/basic_rendering.frag";
    #endif
#elif __APPLE__
    // apple
    std::string vertex_shader_path = "/Users/dimitrisstaratzis/Desktop/CG_Project/Data/Shaders/basic_rendering.vert";
    std::string fragment_shader_path = "/Users/dimitrisstaratzis/Desktop/CG_Project/Data/Shaders/basic_rendering.frag";

#elif __linux__
    // linux
    std::string vertex_shader_path = "Data/Shaders/basic_rendering.vert";
    std::string fragment_shader_path = "Data/Shaders/basic_rendering.frag";

#endif

    m_geometry_rendering_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
    m_geometry_rendering_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
    initialized = m_geometry_rendering_program.CreateProgram();
    m_geometry_rendering_program.LoadUniform("uniform_projection_matrix");
    m_geometry_rendering_program.LoadUniform("uniform_view_matrix");
    m_geometry_rendering_program.LoadUniform("uniform_model_matrix");
    m_geometry_rendering_program.LoadUniform("uniform_normal_matrix");
    m_geometry_rendering_program.LoadUniform("uniform_diffuse");
    m_geometry_rendering_program.LoadUniform("uniform_specular");
    m_geometry_rendering_program.LoadUniform("uniform_shininess");
    m_geometry_rendering_program.LoadUniform("alpha_value");
    m_geometry_rendering_program.LoadUniform("uniform_has_texture");
    m_geometry_rendering_program.LoadUniform("diffuse_texture");
    m_geometry_rendering_program.LoadUniform("uniform_camera_position");
    
	// Light Source Uniforms
    m_geometry_rendering_program.LoadUniform("uniform_light_projection_matrix");
    m_geometry_rendering_program.LoadUniform("uniform_light_view_matrix");
    m_geometry_rendering_program.LoadUniform("uniform_light_position");
    m_geometry_rendering_program.LoadUniform("uniform_light_direction");
    m_geometry_rendering_program.LoadUniform("uniform_light_color");
    m_geometry_rendering_program.LoadUniform("uniform_light_umbra");
    m_geometry_rendering_program.LoadUniform("uniform_light_penumbra");
    m_geometry_rendering_program.LoadUniform("uniform_cast_shadows");
    m_geometry_rendering_program.LoadUniform("shadowmap_texture");

    // Create and Compile Particle Shader
#ifdef _WIN32
    vertex_shader_path = "../Data/Shaders/particle_rendering.vert";
    fragment_shader_path = "../Data/Shaders/particle_rendering.frag";
    m_particle_rendering_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
    m_particle_rendering_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
    initialized = initialized && m_particle_rendering_program.CreateProgram();
    m_particle_rendering_program.LoadUniform("uniform_view_matrix");
    m_particle_rendering_program.LoadUniform("uniform_projection_matrix");
#elif __APPLE__
    vertex_shader_path = "/Users/dimitrisstaratzis/Desktop/CG_Project/Data/Shaders/particle_rendering.vert";
    fragment_shader_path = "/Users/dimitrisstaratzis/Desktop/CG_Project/Data/Shaders/particle_rendering.frag";
    m_particle_rendering_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
    m_particle_rendering_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
    initialized = initialized && m_particle_rendering_program.CreateProgram();
    m_particle_rendering_program.LoadUniform("uniform_view_matrix");
    m_particle_rendering_program.LoadUniform("uniform_projection_matrix");
#elif __linux__
    vertex_shader_path = "Data/Shaders/particle_rendering.vert";
    fragment_shader_path = "Data/Shaders/particle_rendering.frag";
    m_particle_rendering_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
    m_particle_rendering_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
    initialized = initialized && m_particle_rendering_program.CreateProgram();
    m_particle_rendering_program.LoadUniform("uniform_view_matrix");
    m_particle_rendering_program.LoadUniform("uniform_projection_matrix");
#endif


    // Post Processing Program
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        vertex_shader_path = "../Data/Shaders/postproc.vert";
        fragment_shader_path = "../Data/Shaders/postproc.frag";
    #endif
#elif __APPLE__
    // apple
    vertex_shader_path = "/Users/dimitrisstaratzis/Desktop/CG_Project/Data/Shaders/postproc.vert";
    fragment_shader_path = "/Users/dimitrisstaratzis/Desktop/CG_Project/Data/Shaders/postproc.frag";

#elif __linux__
    // linux
    vertex_shader_path = "Data/Shaders/postproc.vert";
    fragment_shader_path = "Data/Shaders/postproc.frag";

#endif
    m_postprocess_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
    m_postprocess_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
    initialized = initialized && m_postprocess_program.CreateProgram();
    m_postprocess_program.LoadUniform("uniform_texture");
    m_postprocess_program.LoadUniform("uniform_time");
    m_postprocess_program.LoadUniform("uniform_depth");
    m_postprocess_program.LoadUniform("uniform_projection_inverse_matrix");
    
	m_postprocess_program.LoadUniform("game_over");
	m_postprocess_program.LoadUniform("place_mode");

    m_postprocess_program.LoadUniform("blue_towers");
    m_postprocess_program.LoadUniform("red_towers");
    m_postprocess_program.LoadUniform("coins_left");

    m_postprocess_program.LoadUniform("lose_coins_effect");
    m_postprocess_program.LoadUniform("lose_coins_effect_frame");
    m_postprocess_program.LoadUniform("lose_coins_effect_duration");


    // Shadow mapping Program
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        vertex_shader_path = "../Data/Shaders/shadow_map_rendering.vert";
        fragment_shader_path = "../Data/Shaders/shadow_map_rendering.frag";
    #endif
#elif __APPLE__
    // apple
    vertex_shader_path = "/Users/dimitrisstaratzis/Desktop/CG_Project/Data/Shaders/shadow_map_rendering.vert";
    fragment_shader_path = "/Users/dimitrisstaratzis/Desktop/CG_Project/Data/Shaders/shadow_map_rendering.frag";

#elif __linux__
    // linux
    vertex_shader_path = "Data/Shaders/shadow_map_rendering.vert";
    fragment_shader_path = "Data/Shaders/shadow_map_rendering.frag";

#endif

    m_spot_light_shadow_map_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
    m_spot_light_shadow_map_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
    initialized = initialized && m_spot_light_shadow_map_program.CreateProgram();
    m_spot_light_shadow_map_program.LoadUniform("uniform_projection_matrix");
    m_spot_light_shadow_map_program.LoadUniform("uniform_view_matrix");
    m_spot_light_shadow_map_program.LoadUniform("uniform_model_matrix");

    return initialized;
}
bool Renderer::ReloadShaders()
{
	bool reloaded = true;

	reloaded = reloaded && m_geometry_rendering_program.ReloadProgram();
	reloaded = reloaded && m_postprocess_program.ReloadProgram();
	reloaded = reloaded && m_spot_light_shadow_map_program.ReloadProgram();
    // Reload Particle Shader
    reloaded = reloaded && m_particle_rendering_program.ReloadProgram();

	return reloaded;
}

bool Renderer::InitDeferredShaderBuffers()
{
	// generate texture handles 
	glGenTextures(1, &m_fbo_texture);
	glGenTextures(1, &m_fbo_depth_texture);

	// framebuffer to link everything together
	glGenFramebuffers(1, &m_fbo);

	return ResizeBuffers(m_screen_width, m_screen_height);
}

// resize post processing textures and save the screen size
bool Renderer::ResizeBuffers(int width, int height)
{
	m_screen_width = width;
	m_screen_height = height;

	// texture
	glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_screen_width, m_screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// depth texture
	glBindTexture(GL_TEXTURE_2D, m_fbo_depth_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_screen_width, m_screen_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// framebuffer to link to everything together
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fbo_depth_texture, 0);

	GLenum status = Tools::CheckFramebufferStatus(m_fbo);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		return false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_projection_matrix = glm::perspective(glm::radians(60.f), width / (float)height, 0.1f, 1500.0f);
    m_view_matrix = glm::lookAt(m_camera_position, m_camera_target_position, m_camera_up_vector);

	return true;
}

// Initialize the light sources
bool Renderer::InitLightSources()
{
    // Initialize the spot light
    m_spotlight_node.SetPosition(glm::vec3(11, 18, 2));
    m_spotlight_node.SetTarget(glm::vec3(10, 0, 10));
    m_spotlight_node.SetColor(100.f * glm::vec3(140, 140, 140) / 255.f);
    m_spotlight_node.SetConeSize(88, 88);
    m_spotlight_node.CastShadow(true);

	return true;
}

// Load Geometric Meshes
bool Renderer::InitGeometricMeshes()
{
	bool initialized = true;
	OBJLoader loader;
	// load terrain

#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        auto mesh = loader.load("../Assets/Terrain/terrain.obj");
    #endif
#elif __APPLE__
    // apple
   auto mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Terrain/terrain.obj");

#elif __linux__
    // linux
    auto mesh = loader.load("Assets/Terrain/terrain.obj");

#endif

    if (mesh != nullptr)
    {
        m_terrain = new GeometryNode();
        m_terrain->Init(mesh);
    }
    else
        initialized = false;

    // load treasure
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        mesh = loader.load("../Assets/Treasure/treasure_chest.obj");
    #endif
#elif __APPLE__
    // apple
   mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Treasure/treasure_chest.obj");

#elif __linux__
    // linux
    mesh = loader.load("Assets/Treasure/treasure_chest.obj");

#endif
    if (mesh != nullptr)
    {
        m_chest_object = new GeometryNode();
        m_chest_object->Init(mesh);
    }
    else
        initialized = false;

    chest = new Chest(m_chest_object);

	/*
    // load menu
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        mesh = loader.load("../Assets/Menu/menu.obj");
    #endif
#elif __APPLE__
    // apple
   mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Menu/road.obj");

#elif __linux__
    // linux
    mesh = loader.load("Assets/Terrain/road.obj");
#endif

    if (mesh != nullptr)
    {
        m_menu_object = new GeometryNode();
        m_menu_object->Init(mesh);
    }
    else
        initialized = false;

    menu = new Menu(m_menu_object);
	*/

    // load tower1
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        mesh = loader.load("../Assets/MedievalTower/tower.obj");
    #endif
#elif __APPLE__
    // apple
   mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/MedievalTower/tower.obj");

#elif __linux__
    // linux
    mesh = loader.load("Assets/MedievalTower/tower.obj");
#endif

    if (mesh != nullptr)
    {
        m_tower_object = new GeometryNode();
        m_tower_object->Init(mesh);
    }
    else
        initialized = false;

    // load tower2
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        mesh = loader.load("../Assets/MedievalTower/tower2.obj");
    #endif
#elif __APPLE__
    // apple
   mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/MedievalTower/tower2.obj");

#elif __linux__
    // linux
    mesh = loader.load("Assets/MedievalTower/tower2.obj");
#endif

    if (mesh != nullptr)
    {
        m_tower2_object = new GeometryNode();
        m_tower2_object->Init(mesh);
    }
    else
        initialized = false;

    // load tile
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        mesh = loader.load("../Assets/Terrain/road.obj");
    #endif
#elif __APPLE__
    // apple
   mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Terrain/road.obj");

#elif __linux__
    // linux
    mesh = loader.load("Assets/Terrain/road.obj");
#endif

    if (mesh != nullptr)
    {
        m_road_object = new GeometryNode();

#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        readRoad("../Data/road.map");
    #endif
#elif __APPLE__
    // apple
        readRoad("/Users/dimitrisstaratzis/Desktop/CG_Project/Data/road.map");

#elif __linux__
    // linux
        readRoad("Data/road.map");
#endif
        m_road_object->Init(mesh);
    }
    else
        initialized = false;

    // load cannonball
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        mesh = loader.load("../Assets/Various/cannonball.obj");
    #endif
#elif __APPLE__
    // apple
   mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Various/cannonball.obj");

#elif __linux__
    // linux
    mesh = loader.load("Assets/Various/cannonball.obj");

#endif
    if (mesh != nullptr)
    {
        m_cannonball_object = new GeometryNode();
        m_cannonball_object->Init(mesh);
    }
    else
        initialized = false;



    // load rocket
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        mesh = loader.load("../Assets/Various/rocket.obj");
    #endif
#elif __APPLE__
    // apple
   mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Various/rocket.obj");

#elif __linux__
    // linux
    mesh = loader.load("Assets/Various/rocket.obj");

#endif
    if (mesh != nullptr)
    {
        m_rocket_object = new GeometryNode();
        m_rocket_object->Init(mesh);
    }
    else
        initialized = false;



    // load green_tile
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        mesh = loader.load("../Assets/Various/plane_green.obj");
    #endif
#elif __APPLE__
    // apple
   mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Various/plane_green.obj");

#elif __linux__
    // linux
    mesh = loader.load("Assets/Various/plane_green.obj");
#endif

    if (mesh != nullptr)
    {
        m_green_tile = new GeometryNode();
        m_green_tile->Init(mesh);
    }
    else
        initialized = false;


    // load red_tile
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        mesh = loader.load("../Assets/Various/plane_red.obj");
    #endif
#elif __APPLE__
    // apple
   mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Various/plane_red.obj");

#elif __linux__
    // linux
    mesh = loader.load("Assets/Various/plane_red.obj");
#endif

    if (mesh != nullptr)
    {
        m_red_tile = new GeometryNode();
        m_red_tile->Init(mesh);
    }
    else
        initialized = false;


    // load boss pirate
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        mesh = loader.load("../Assets/Pirate/pirate_body_boss.obj");
    #endif
#elif __APPLE__
    // apple
   mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Pirate/pirate_body_boss.obj");

#elif __linux__
    // linux
    mesh = loader.load("Assets/Pirate/pirate_body_boss.obj");
#endif

    if (mesh != nullptr)
    {
        m_skeleton_boss_body = new GeometryNode();
        m_skeleton_boss_body->Init(mesh);
    }
    else
        initialized = false;


    // load pirate
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        mesh = loader.load("../Assets/Pirate/pirate_body.obj");
    #endif
#elif __APPLE__
    // apple
   mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Pirate/pirate_body.obj");

#elif __linux__
    // linux
    mesh = loader.load("Assets/Pirate/pirate_body.obj");
#endif

    if (mesh != nullptr)
    {
        m_skeleton_object[0] = new GeometryNode();
        m_skeleton_object[0]->Init(mesh);
    }
    else
        initialized = false;

#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        mesh = loader.load("../Assets/Pirate/pirate_arm.obj");
    #endif
#elif __APPLE__
    // apple
   mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Pirate/pirate_arm.obj");

#elif __linux__
    // linux
    mesh = loader.load("Assets/Pirate/pirate_arm.obj");
#endif

    if (mesh != nullptr)
    {
        m_skeleton_object[1] = new GeometryNode();
        m_skeleton_object[1]->Init(mesh);
    }
    else
        initialized = false;

#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
         mesh = loader.load("../Assets/Pirate/pirate_left_foot.obj");
    #endif
#elif __APPLE__
    // apple
    mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Pirate/pirate_left_foot.obj");

#elif __linux__
    // linux
     mesh = loader.load("Assets/Pirate/pirate_left_foot.obj");
#endif

    if (mesh != nullptr)
    {
        m_skeleton_object[2] = new GeometryNode();
        m_skeleton_object[2]->Init(mesh);
    }
    else
        initialized = false;

#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
          mesh = loader.load("../Assets/Pirate/pirate_right_foot.obj");
    #endif
#elif __APPLE__
    // apple
     mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Pirate/pirate_right_foot.obj");

#elif __linux__
    // linux
    mesh = loader.load("Assets/Pirate/pirate_right_foot.obj");
#endif

    if (mesh != nullptr)
    {
        m_skeleton_object[3] = new GeometryNode();
        m_skeleton_object[3]->Init(mesh);
    }
    else
        initialized = false;

	// load green health bar
#ifdef _WIN32
	//define something for Windows (32-bit and 64-bit, this part is common)
#ifdef _WIN64
	mesh = loader.load("../Assets/Various/health_bar_green.obj");
#endif
#elif __APPLE__
	// apple
	mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Various/health_bar_green.obj");

#elif __linux__
	// linux
	mesh = loader.load("Assets/Various/health_bar_green.obj");
#endif

	if (mesh != nullptr)
	{
        m_skeleton_object[4] = new GeometryNode();
        m_skeleton_object[4]->Init(mesh);
	}
	else
		initialized = false;

	// load red health bar
#ifdef _WIN32
	//define something for Windows (32-bit and 64-bit, this part is common)
#ifdef _WIN64
	mesh = loader.load("../Assets/Various/health_bar_red.obj");
#endif
#elif __APPLE__
	// apple
	mesh = loader.load("/Users/dimitrisstaratzis/Desktop/CG_Project/Assets/Various/health_bar_red.obj");

#elif __linux__
	// linux
	mesh = loader.load("Assets/Various/health_bar_red.obj");
#endif

	if (mesh != nullptr)
	{
        m_skeleton_object[5] = new GeometryNode();
        m_skeleton_object[5]->Init(mesh);
	}
	else
		initialized = false;

	return initialized;
}

void Renderer::SetRenderingMode(RENDERING_MODE mode)
{
	m_rendering_mode = mode;
}

// Render the Scene
void Renderer::Render()
{
	// Draw the geometry to the shadow maps
    RenderShadowMaps();

	// Draw the geometry
    RenderGeometry();

	// Render to screen
    RenderToOutFB();

	GLenum error = Tools::CheckGLError();
	if (error != GL_NO_ERROR)
	{
        printf("Renderer:Draw GL Error\n");
        //system("pause");
	}
}

void Renderer::RenderShadowMaps()
{
	// if the light source casts shadows
	if (m_spotlight_node.GetCastShadowsStatus())
	{
		int m_depth_texture_resolution = m_spotlight_node.GetShadowMapResolution();

		// bind the framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, m_spotlight_node.GetShadowMapFBO());
		glViewport(0, 0, m_depth_texture_resolution, m_depth_texture_resolution);
		GLenum drawbuffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, drawbuffers);
		
		// clear depth buffer
		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		
		// Bind the shadow mapping program
		m_spot_light_shadow_map_program.Bind();

		// pass the projection and view matrix to the uniforms
		glUniformMatrix4fv(m_spot_light_shadow_map_program["uniform_projection_matrix"], 1, GL_FALSE, glm::value_ptr(m_spotlight_node.GetProjectionMatrix()));
		glUniformMatrix4fv(m_spot_light_shadow_map_program["uniform_view_matrix"], 1, GL_FALSE, glm::value_ptr(m_spotlight_node.GetViewMatrix()));

		// draw the terrain object
        DrawGeometryNodeToShadowMap(m_terrain, m_terrain_transformation_matrix, m_terrain_transformation_normal_matrix);

		// draw the treasure object
        if(!GAME_OVER){
            DrawGeometryNodeToShadowMap(chest->getGeometricNode(), chest->getGeometricTransformationMatrix(), chest->getGeometricTransformationNormalMatrix());
        }
		// draw towers
		for (auto &tower : m_towers)
		{
			DrawGeometryNodeToShadowMap(tower.getGeometricNode(), tower.getGeometricTransformationMatrix(), tower.getGeometricTransformationNormalMatrix());
		}

        //draw cannonballs
        for (auto &cannonball : m_cannonballs)
        {
            DrawGeometryNodeToShadowMap(cannonball.getGeometricNode(), cannonball.getGeometricTransformationMatrix(), cannonball.getGeometricTransformationNormalMatrix());
        }

        //draw rocket
        for (auto &rocket : m_rockets)
        {
            DrawGeometryNodeToShadowMap(rocket.getGeometricNode(), rocket.getGeometricTransformationMatrix(), rocket.getGeometricTransformationNormalMatrix());
        }

		// draw tiles
		for (auto &tile : m_road)
		{
			DrawGeometryNodeToShadowMap(tile.getGeometricNode(), tile.getGeometricTransformationMatrix(), tile.getGeometricTransformationNormalMatrix());
		}

		// draw the green tile
		DrawGeometryNodeToShadowMap(m_player_tile, m_player_tile_transformation_matrix, m_player_tile_transformation_normal_matrix);

        // draw the pirate
		for (auto &skeleton : m_skeletons)
		{
			// Don' t render shadow map for the health bars
			for (size_t i = 0; i < 4; i++)
			{
                if(skeleton.getPosition().z > -1 && skeleton.will_render())
                    DrawGeometryNodeToShadowMap(skeleton.getGeometricNode()[i], skeleton.getGeometricTransformationMatrix()[i], skeleton.getGeometricTransformationNormalMatrix()[i]);
			}
		}


		glBindVertexArray(0);

		// Unbind shadow mapping program
		m_spot_light_shadow_map_program.Unbind();
		
		glDisable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void Renderer::RenderGeometry()
{
	// Bind the Intermediate framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glViewport(0, 0, m_screen_width, m_screen_height);
	GLenum drawbuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawbuffers);
	
	// clear color and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	switch (m_rendering_mode)
	{
	case RENDERING_MODE::TRIANGLES:
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case RENDERING_MODE::LINES:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case RENDERING_MODE::POINTS:
		glPointSize(2);
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		break;
	};
	
	// Bind the geometry rendering program
	m_geometry_rendering_program.Bind();

	// pass camera parameters to uniforms
	glUniformMatrix4fv(m_geometry_rendering_program["uniform_projection_matrix"], 1, GL_FALSE, glm::value_ptr(m_projection_matrix));
	glUniformMatrix4fv(m_geometry_rendering_program["uniform_view_matrix"], 1, GL_FALSE, glm::value_ptr(m_view_matrix));
	glUniform3f(m_geometry_rendering_program["uniform_camera_position"], m_camera_position.x, m_camera_position.y, m_camera_position.z);

	// pass the light source parameters to uniforms
    glm::vec3 light_position = m_spotlight_node.GetPosition();
    glm::vec3 light_direction = m_spotlight_node.GetDirection();
    glm::vec3 light_color = m_spotlight_node.GetColor();
    glUniformMatrix4fv(m_geometry_rendering_program["uniform_light_projection_matrix"], 1, GL_FALSE, glm::value_ptr(m_spotlight_node.GetProjectionMatrix()));
    glUniformMatrix4fv(m_geometry_rendering_program["uniform_light_view_matrix"], 1, GL_FALSE, glm::value_ptr(m_spotlight_node.GetViewMatrix()));
	glUniform3f(m_geometry_rendering_program["uniform_light_position"], light_position.x, light_position.y, light_position.z);
	glUniform3f(m_geometry_rendering_program["uniform_light_direction"], light_direction.x, light_direction.y, light_direction.z);
	glUniform3f(m_geometry_rendering_program["uniform_light_color"], light_color.x, light_color.y, light_color.z);
    glUniform1f(m_geometry_rendering_program["uniform_light_umbra"], m_spotlight_node.GetUmbra());
    glUniform1f(m_geometry_rendering_program["uniform_light_penumbra"], m_spotlight_node.GetPenumbra());
    glUniform1i(m_geometry_rendering_program["uniform_cast_shadows"], (m_spotlight_node.GetCastShadowsStatus())? 1 : 0);

	// Set the sampler2D uniform to use texture unit 1
	glUniform1i(m_geometry_rendering_program["shadowmap_texture"], 1);
	// Bind the shadow map texture to texture unit 1
	glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, (m_spotlight_node.GetCastShadowsStatus()) ? m_spotlight_node.GetShadowMapDepthTexture() : 0);
	
	// Enable Texture Unit 0
	glUniform1i(m_geometry_rendering_program["uniform_diffuse_texture"], 0);
	glActiveTexture(GL_TEXTURE0);

	// draw the heart(s) object
	//DrawGeometryNode(m_heart_object, m_heart_object_transformation_matrix, m_heart_object_transformation_normal_matrix);

	// draw the terrain object
    DrawGeometryNode(m_terrain, m_terrain_transformation_matrix, m_terrain_transformation_normal_matrix);

    //Draw menu
    //DrawGeometryNode(menu->getGeometricNode(), menu->getGeometricTransformationMatrix(), menu->getGeometricTransformationNormalMatrix());
	
	// draw the treasure object
    if(!GAME_OVER){
        DrawGeometryNode(chest->getGeometricNode(), chest->getGeometricTransformationMatrix(), chest->getGeometricTransformationNormalMatrix());
    }

    //draw cannonball
    for (auto &cannonball : m_cannonballs)
    {
        DrawGeometryNode(cannonball.getGeometricNode(), cannonball.getGeometricTransformationMatrix(), cannonball.getGeometricTransformationNormalMatrix());
    }

    //draw rockets
    for (auto &rocket : m_rockets)
    {
        DrawGeometryNode(rocket.getGeometricNode(), rocket.getGeometricTransformationMatrix(), rocket.getGeometricTransformationNormalMatrix());
    }

	// draw towers
    for (auto &tower : m_towers)
	{
        DrawGeometryNode(tower.getGeometricNode(), tower.getGeometricTransformationMatrix(), tower.getGeometricTransformationNormalMatrix());
	}

	// draw tiles
	for (auto &tile : m_road)
	{
		DrawGeometryNode(tile.getGeometricNode(), tile.getGeometricTransformationMatrix(), tile.getGeometricTransformationNormalMatrix());
	}

	// draw the green tile
    DrawGeometryNode(m_player_tile, m_player_tile_transformation_matrix, m_player_tile_transformation_normal_matrix);

	// draw the pirate
	for (auto &skeleton : m_skeletons)
	{
		for (size_t i = 0; i < 6; i++)
		{
			// Don' t render the health bar when skeleton is dead
			// Don' t render the red health bar when skeleton has full health
			if (i == 5 && skeleton.get_health() == skeleton.get_max_health())
			{
				continue;
			}

			if (skeleton.get_health() == 0 && (i == 5 || i == 4))
			{
				continue;
			}
            
			if (skeleton.getPosition().z > -1 && skeleton.will_render())
			{
				//std::cout<<m_level<<" <-"<<std::endl;
				if (m_level % 3 == 0 && i == 0)
				{
					DrawGeometryNode(m_skeleton_boss_body, skeleton.getGeometricTransformationMatrix()[i], skeleton.getGeometricTransformationNormalMatrix()[i]);
				}
				else
				{
					DrawGeometryNode(skeleton.getGeometricNode()[i], skeleton.getGeometricTransformationMatrix()[i], skeleton.getGeometricTransformationNormalMatrix()[i]);
				}
			}
		}
	}

	glBindVertexArray(0);
	m_geometry_rendering_program.Unbind();

    // Render Particles
    m_particle_rendering_program.Bind();

    glUniformMatrix4fv(m_particle_rendering_program["uniform_projection_matrix"], 1, GL_FALSE, glm::value_ptr(m_projection_matrix));
    glUniformMatrix4fv(m_particle_rendering_program["uniform_view_matrix"], 1, GL_FALSE, glm::value_ptr(m_view_matrix));

    for(int i=0; i<m_particle_emitters.size(); i++)
    {
        m_particle_emitters[i].Render();

    }

    m_particle_rendering_program.Unbind();

	glDisable(GL_DEPTH_TEST);
	if(m_rendering_mode != RENDERING_MODE::TRIANGLES)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPointSize(1.0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawGeometryNode(GeometryNode* node, glm::mat4 model_matrix, glm::mat4 normal_matrix)
{
    if (node == m_player_tile)
	{
		node->alpha = 0.5f;
	}
	else
	{
		node->alpha = 1.f;
	}

	glBindVertexArray(node->m_vao);
	glUniformMatrix4fv(m_geometry_rendering_program["uniform_model_matrix"], 1, GL_FALSE, glm::value_ptr(model_matrix));
	glUniformMatrix4fv(m_geometry_rendering_program["uniform_normal_matrix"], 1, GL_FALSE, glm::value_ptr(normal_matrix));
	for (int j = 0; j < node->parts.size(); j++)
	{
		glm::vec3 diffuseColor = node->parts[j].diffuseColor;
		glm::vec3 specularColor = node->parts[j].specularColor;
		float shininess = node->parts[j].shininess;
		glUniform3f(m_geometry_rendering_program["uniform_diffuse"], diffuseColor.r, diffuseColor.g, diffuseColor.b);
		glUniform3f(m_geometry_rendering_program["uniform_specular"], specularColor.r, specularColor.g, specularColor.b);
		glUniform1f(m_geometry_rendering_program["uniform_shininess"], shininess);
		glUniform1f(m_geometry_rendering_program["uniform_has_texture"], (node->parts[j].textureID > 0) ? 1.0f : 0.0f);
		glUniform1f(m_geometry_rendering_program["alpha_value"], node->alpha);
		glBindTexture(GL_TEXTURE_2D, node->parts[j].textureID);

		glDrawArrays(GL_TRIANGLES, node->parts[j].start_offset, node->parts[j].count);
	}
}

void Renderer::DrawGeometryNodeToShadowMap(GeometryNode* node, glm::mat4 model_matrix, glm::mat4 normal_matrix)
{
	glBindVertexArray(node->m_vao);
	glUniformMatrix4fv(m_spot_light_shadow_map_program["uniform_model_matrix"], 1, GL_FALSE, glm::value_ptr(model_matrix));
	for (int j = 0; j < node->parts.size(); j++)
	{
		glDrawArrays(GL_TRIANGLES, node->parts[j].start_offset, node->parts[j].count);
	}
}

void Renderer::RenderToOutFB()
{
	// Bind the default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_screen_width, m_screen_height);
	
	// clear the color and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// disable depth testing and blending
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	// bind the post processing program
	m_postprocess_program.Bind();
	
	glBindVertexArray(m_vao_fbo);
	// Bind the intermediate color image to texture unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
	glUniform1i(m_postprocess_program["uniform_texture"], 0);
	// Bind the intermediate depth buffer to texture unit 1
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_fbo_depth_texture);	
	glUniform1i(m_postprocess_program["uniform_depth"], 1);
	
	glUniform1i(m_postprocess_program["game_over"], GAME_OVER ? 1 : 0);
	glUniform1i(m_postprocess_program["place_mode"], m_default_tower ? 1 : 0);

	glUniform1i(m_postprocess_program["blue_towers"], m_blue_tower_counter);
	glUniform1i(m_postprocess_program["red_towers"], m_red_tower_counter);
    glUniform1i(m_postprocess_program["coins_left"], chest->getCoinsLeft());

	if (m_lose_coins_effect == 1)
	{
		if (m_lose_coins_effect_frame == m_lose_coins_effect_duration)
		{
			m_lose_coins_effect = 0;
			m_lose_coins_effect_frame = 0;
		}
		else
		{
			m_lose_coins_effect_frame++;
		}
	}
    glUniform1i(m_postprocess_program["lose_coins_effect"], m_lose_coins_effect);
    glUniform1i(m_postprocess_program["lose_coins_effect_frame"], m_lose_coins_effect_frame);
    glUniform1i(m_postprocess_program["lose_coins_effect_duration"], m_lose_coins_effect_duration);

	glUniform1f(m_postprocess_program["uniform_time"], m_continuous_time);
	glm::mat4 projection_inverse_matrix = glm::inverse(m_projection_matrix);
	glUniformMatrix4fv(m_postprocess_program["uniform_projection_inverse_matrix"], 1, GL_FALSE, glm::value_ptr(projection_inverse_matrix));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);

	// Unbind the post processing program
	m_postprocess_program.Unbind();
}

void Renderer::CameraMoveForward(bool enable)
{
    m_camera_movement.x = (enable)? 1 : 0;
}
void Renderer::CameraMoveBackWard(bool enable)
{
	m_camera_movement.x = (enable) ? -1 : 0;
}

void Renderer::CameraMoveLeft(bool enable)
{
	m_camera_movement.y = (enable) ? -1 : 0;
}
void Renderer::CameraMoveRight(bool enable)
{
	m_camera_movement.y = (enable) ? 1 : 0;
}

void Renderer::CameraLook(glm::vec2 lookDir)
{
	m_camera_look_angle_destination = glm::vec2(1, -1) * lookDir;
}

void Renderer::MovePlayer(int dx, int dz) {
    if (m_player_tile_position.x + dx >= 0 && m_player_tile_position.x + dx <= 19)
	{
        m_player_tile_position.x += dx;
	}
    if (m_player_tile_position.z + dz >= 0 && m_player_tile_position.z + dz <= 19)
	{
        m_player_tile_position.z += dz;
    }
}

void Renderer::PlaceTower() {
	if (isValidTowerPos() && !GAME_OVER) //place towers every 10 seconds. Allow player to build two towers at first
	{
		
		if (m_default_tower && m_blue_tower_counter > 0)
		{
			m_towers.emplace_back(m_player_tile_position, m_tower_object, 2, 1.0f, false);
			m_blue_tower_counter--;
		}
		else if (!m_default_tower && m_red_tower_counter > 0)
		{
			m_towers.emplace_back(m_player_tile_position, m_tower2_object, 2, 2.0f, true);
			m_red_tower_counter--;
		}
	}
}

void Renderer::setDefaultTower(bool flag)
{
    m_default_tower = flag;
}

bool Renderer::isValidTowerPos() {
	for (auto &tower : m_towers) {
        if (m_player_tile_position.x == tower.getPosition().x
            && m_player_tile_position.z == tower.getPosition().z)
		{
            return false;
		}
	}
	for (auto &tile : m_road) {
        if (m_player_tile_position.x == tile.getPosition().x
            && m_player_tile_position.z == tile.getPosition().z)
		{
            return false;
		}
	}
    return true;
}

void Renderer::MoveSkeleton(float dt) {
	for (auto &skeleton : m_skeletons) {
		skeleton.Move(dt, m_continuous_time);
	}
    //RemoveSkeleton();
}

void Renderer::RemoveSkeleton() {
	for (size_t i = 0; i < m_skeletons.size();)
	{
		glm::vec3 delta_skeleton = m_road[m_road.size() - 1].getPosition() - m_skeletons[i].getPosition();

        if (std::abs(delta_skeleton.x) < .3f && std::abs(delta_skeleton.z) < .3f) {
			m_skeletons.erase(m_skeletons.begin() + i);
		}
		else
		{
			i++;
		}
	}
}

bool Renderer::readRoad(const char *road)
{
    std::fstream in(road, std::ios::in);
    if (!in)
    {
        std::cerr << "Error: cannot open file " << road << std::endl;
        return false;
    }

    int x, z;

    while (in >> x >> z)
        m_road.emplace_back(2, 2, glm::vec3(x, 0.01, z), m_road_object);
    in.close();
    std::cout << "DONE reading the road file" << std::endl;

    return true;
}

void Renderer::shoot(float dt)
{
    for (auto & tower : m_towers)
    {

        int target = tower.shoot_closest(m_skeletons, TERRAIN_WIDTH, TERRAIN_HEIGHT, dt);
        if(target != -1)
        {
            if(!tower.is_following_target())
            {
                m_cannonballs.emplace_back(tower.getPosition(), m_cannonball_object, target, 5.5f, m_skeletons[target].getPosition());
				Audio::PlayAudio("Cannon.wav");
            }else
            {
                m_rockets.emplace_back(tower.getPosition(), m_rocket_object, target, 2.5f, m_skeletons[target].getPosition());
				Audio::PlayAudio("rocket2_low_pitch_fast.wav");
            }

        }

    }
}

void Renderer::SpawnNewSkeletons()
{
	m_pirate_position = glm::vec3(0, -1.f, 0);
    if(m_level == 10 && !m_path_has_changed)
    {
        m_path_has_changed=true;
        InitiallizeNextPath();
#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        readRoad("../Data/road2.map");
    #endif
#elif __APPLE__
    // apple
        readRoad("/Users/dimitrisstaratzis/Desktop/CG_Project/Data/road2.map");

#elif __linux__
    // linux
        readRoad("Data/road2.map");
#endif
    }
	
    if (m_level % 3 == 0)
	{
        m_skeletons.emplace_back(m_pirate_position, 1, (float)rand() / RAND_MAX, m_road, m_skeleton_object, 12 * m_level, 0.12f, 1.f, true);
	}
	else
    {
		m_skeleton_counter +=((m_level % 2 == 0) ? 1 : 0);
		for (int i = 0; i < m_road.size() && i < m_skeleton_counter; i++)
		{
            //std::cout<<level<<std::endl;
            m_skeletons.emplace_back(m_pirate_position, 1, (float)rand() / RAND_MAX, m_road, m_skeleton_object, 3 * m_level, 0.06f, 2.f, false);
			m_pirate_position.z -= 2;
		}
    }

}

void Renderer::RemoveTower()
{
	if (!GAME_OVER)
	{
		for (int i = 0; i < m_towers.size(); i++)
		{
			if (m_towers[i].getPosition() == m_player_tile_position)
			{
				m_towers[i].set_to_be_removed(true);
				
				if (m_towers[i].is_following_target())
				{
					if (++m_red_tower_replace_counter == 2)
					{
						m_red_tower_replace_counter = 0;
						m_red_tower_counter++;
					}
				}
				else
				{
					if (++m_blue_tower_replace_counter == 2)
					{
						m_blue_tower_replace_counter = 0;
						m_blue_tower_counter++;
					}
				}
			}
		}
	}
}


void Renderer::InitiallizeNextPath()
{
    m_blue_timer = 0.f;
    m_red_timer = 0.f;
    m_road.clear();
    m_level=1;
    m_skeletons.clear();
    m_towers.clear();
    m_blue_tower_counter = 2;
    m_red_tower_counter = 1;
    chest->add_coins(100);
}

void Renderer::setMichaelJacksonMode(bool flag)
{
    MichaelJacksonMode=flag;
}

bool Renderer::getMichaelJacksonMode()
{
    return MichaelJacksonMode;
}

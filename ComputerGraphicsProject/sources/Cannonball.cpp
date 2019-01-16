#include "../headers/Cannonball.h"
#include <map>

Cannonball::Cannonball()
{

}

Cannonball::Cannonball(glm::vec3 position, GeometryNode* g_node, int target, float speed) {
    m_geometric_node = g_node;
    position.y = 2.5f;
    setPosition(position);
    m_radius = 0.2f;
    m_target = target;
    m_center_of_sphere = m_position;
    m_speed = speed;

}

void Cannonball::setPosition(glm::vec3 position) {
    m_position = position;
    m_geometric_transformation_matrix =
        glm::translate(glm::mat4(1.f), getPosition()) *
        glm::scale(glm::mat4(1.f), glm::vec3(0.2f));
    m_geometric_transformation_normal_matrix = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_geometric_transformation_matrix))));
}

bool Cannonball::update(float dt, std::vector<Skeleton> &skeletons, float gravity)
{
    bool a=false;
    bool b=false;
    bool c=false;
    bool d=false;

    if (skeletons.size() <= m_target)
        {
            return false;
        }

    glm::vec3 delta_cannonball = skeletons[m_target].getPosition() - m_position;

    if(delta_cannonball.x > 0 && std::abs(delta_cannonball.x) > std::abs(delta_cannonball.z)&& !b)
    {
        m_position.x += dt*m_speed;
        //std::cout << "x>0" <<std::endl;
        a=true;

    }
    if (delta_cannonball.x < 0 && std::abs(delta_cannonball.z) < std::abs(delta_cannonball.x) && !a)
    {
        m_position.x -= dt*m_speed;
        //std::cout << "x<0" <<std::endl;
        b=true;

    }
    if(delta_cannonball.z > 0 && std::abs(delta_cannonball.z) > std::abs(delta_cannonball.x) && !d)
    {
        //std::cout << "z>0" <<std::endl;
        m_position.z += dt*m_speed;
        c=true;

    }
    if (delta_cannonball.z < 0 && std::abs(delta_cannonball.z) > std::abs(delta_cannonball.x) && !c)
    {
        //std::cout << "z<0 "<<std::endl;
        m_position.z -= dt*m_speed;
        d=true;
    }

    m_position.y -= gravity *dt;
    if(m_position.y < 0)
    {
        return false;
    }
    setPosition(m_position);
    m_center_of_sphere = m_position;

    float distance = skeletons[m_target].distance_from(m_center_of_sphere);
    if(distance <= (m_radius + skeletons[m_target].getRadius()))
    {
        skeletons[m_target].lose_health(1);
        return false;
    }
    return true;

}

glm::vec3 Cannonball::getPosition() {
    return m_position;
}

GeometryNode* Cannonball::getGeometricNode() {
    return m_geometric_node;
}

glm::mat4 Cannonball::getGeometricTransformationMatrix()
{
    return m_geometric_transformation_matrix;
}

glm::mat4 Cannonball::getGeometricTransformationNormalMatrix()
{
    return m_geometric_transformation_normal_matrix;
}

Cannonball::~Cannonball()
{
}



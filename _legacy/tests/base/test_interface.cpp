
#include "test_interface.h"
// #include "dart/constraint/BoxedLcpSolver.hpp"
// #include "dart/constraint/BoxedLcpConstraintSolver.hpp"
// #include "dart/constraint/PgsBoxedLcpSolver.hpp"


namespace dart
{

    /***************************************************************************
    *                                                                          *
    *                               UTILITIES                                  *
    *                                                                          *
    ***************************************************************************/

    Eigen::Vector3d toEigenVec3( const tysoc::TVec3& vec )
    {
        return Eigen::Vector3d( vec.x, vec.y, vec.z );
    }

    Eigen::Isometry3d toEigenTransform( const tysoc::TMat4& transform )
    {
        Eigen::Isometry3d _res;
        _res.setIdentity();
        
        auto _rotation = _res.rotation();
        for ( size_t i = 0; i < 3; i++ )
            for ( size_t j = 0; j < 3; j++ )
                _rotation( i, j ) = transform.get( i, j );

        _res.rotate( _rotation );
        _res.translation() = toEigenVec3( transform.getPosition() );

        return _res;
    }

    tysoc::TVec3 fromEigenVec3( const Eigen::Vector3d& vec )
    {
        return tysoc::TVec3( vec.x(), vec.y(), vec.z() );
    }

    tysoc::TMat4 fromEigenTransform( const Eigen::Isometry3d& transform )
    {
        tysoc::TMat4 _res;

        _res.setPosition( fromEigenVec3( transform.translation() ) );
        
        auto _rotation = transform.rotation();
        for ( size_t i = 0; i < 3; i++ )
            for ( size_t j = 0; j < 3; j++ )
                _res.set( i, j, _rotation( i, j ) );

        return _res;
    }

    std::vector< std::string > collectAvailableModels( const std::string& folderpath )
    {
        DIR* _directoryPtr;
        struct dirent* _direntPtr;

        _directoryPtr = opendir( folderpath.c_str() );
        if ( !_directoryPtr )
            return std::vector< std::string >();

        auto _modelfiles = std::vector< std::string >();

        // grab each .xml mjcf model
        while ( _direntPtr = readdir( _directoryPtr ) )
        {
            std::string _fname = _direntPtr->d_name;
            if ( _fname.find( ".xml" ) == std::string::npos )
                continue;

            _modelfiles.push_back( _fname );
        }
        closedir( _directoryPtr );

        return _modelfiles;
    }

    dynamics::ShapePtr createCollisionShape( const ShapeData& shapeData )
    {
        dynamics::Shape* _colshape = nullptr;

        if ( shapeData.type == eShapeType::PLANE )
        {
            // create a simple plane with normal corresponding to the z-axis
            _colshape = new dynamics::PlaneShape( Eigen::Vector3d::UnitZ(), 0.0 );
        }
        else if ( shapeData.type == eShapeType::BOX )
        {
            // seems that the box-shape accepts full width-height-depth instead of halves
            _colshape = new dynamics::BoxShape( toEigenVec3( shapeData.size ) );
        }
        else if ( shapeData.type == eShapeType::SPHERE )
        {
            // pass just the radius (first entry in the size vector)
            _colshape = new dynamics::SphereShape( shapeData.size.x );
        }
        else if ( shapeData.type == eShapeType::CYLINDER )
        {
            // pass just the radius and height, as it seems that cylinders have Z-aligned axes
            _colshape = new dynamics::CylinderShape( shapeData.size.x, shapeData.size.y );
        }
        else if ( shapeData.type == eShapeType::CAPSULE )
        {
            // pass just the radius and height, as it seems that capsules have Z-aligned axes
            _colshape = new dynamics::CapsuleShape( shapeData.size.x, shapeData.size.y );
        }
        else if ( shapeData.type == eShapeType::MESH )
        {
            std::cout << "WARNING> sorry, meshe shapes are not supported yet :(" << std::endl;
        }
        else if ( shapeData.type == eShapeType::NONE )
        {
            // if none is given, then the user wants a dummy shape for some funky functionality
            _colshape = nullptr;
        }

        return dynamics::ShapePtr( _colshape );
    }

    Eigen::Vector3d computeCOMoffset( const ShapeData& shapeData )
    {
        Eigen::Vector3d _res = Eigen::Vector3d( 0.0, 0.0, 0.0 );

        if ( shapeData.type == eShapeType::PLANE )
            _res = Eigen::Vector3d( 0.0, 0.0, 0.0 );

        else if ( shapeData.type == eShapeType::BOX )
            _res = Eigen::Vector3d( 0.0, 0.0, 0.0 );

        else if ( shapeData.type == eShapeType::SPHERE )
            _res = Eigen::Vector3d( 0.0, 0.0, 0.0 );

        else if ( shapeData.type == eShapeType::CYLINDER )
            _res = Eigen::Vector3d( 0.0, 0.0, 0.0 );

        else if ( shapeData.type == eShapeType::CAPSULE )
            _res = Eigen::Vector3d( 0.0, 0.0, 0.0 );

        return _res;
    }

    engine::CIRenderable* createRenderableShape( const ShapeData& shapeData )
    {
        engine::CIRenderable* _renderablePtr = nullptr;

        if ( shapeData.type == eShapeType::PLANE )
        {
            _renderablePtr = engine::CMeshBuilder::createPlane( shapeData.size.x,
                                                                shapeData.size.y );
        }
        else if ( shapeData.type == eShapeType::BOX )
        {
            _renderablePtr = engine::CMeshBuilder::createBox( shapeData.size.x,
                                                              shapeData.size.y,
                                                              shapeData.size.z );
        }
        else if ( shapeData.type == eShapeType::SPHERE )
        {
            _renderablePtr = engine::CMeshBuilder::createSphere( shapeData.size.x );
        }
        else if ( shapeData.type == eShapeType::CYLINDER )
        {
            _renderablePtr = engine::CMeshBuilder::createCylinder( shapeData.size.x,
                                                                   shapeData.size.y );
        }
        else if ( shapeData.type == eShapeType::CAPSULE )
        {
            _renderablePtr = engine::CMeshBuilder::createCapsule( shapeData.size.x,
                                                                  shapeData.size.y );
        }

        if ( !_renderablePtr )
            return nullptr;

        _renderablePtr->material()->ambient = { shapeData.color.x, shapeData.color.y, shapeData.color.z };
        _renderablePtr->material()->diffuse = { shapeData.color.x, shapeData.color.y, shapeData.color.z };
        _renderablePtr->material()->specular = { shapeData.color.x, shapeData.color.y, shapeData.color.z };

        return _renderablePtr;
    }

    TScalar computeMassFromShape( const dynamics::ShapePtr colshape )
    {
        if ( colshape )
            return colshape->getVolume() * DEFAULT_DENSITY;
        
        return 0.0;
    }

    /***************************************************************************
    *                                                                          *
    *                             BODY WRAPPER                                 *
    *                                                                          *
    ***************************************************************************/

    SimBody::SimBody( const std::string& name,
                      dynamics::SkeletonPtr dartSkeletonPtr )
    {
        m_name = name;
        m_parent = nullptr;
        m_graphicsObj = nullptr;
        m_dartSkeletonPtr = dartSkeletonPtr;
    }

    void SimBody::build( SimBody* parent,
                         const ShapeData& shapeData,
                         const JointData& jointData )
    {
        // cache the parent simbody
        m_parent = parent;

        dynamics::BodyNodePtr _parentBodyNodePtr;
        _parentBodyNodePtr = ( m_parent == nullptr ) ? nullptr : parent->node();

        // construct collision shape to be used
        m_dartShapePtr = createCollisionShape( shapeData );

        // construct body properties
        dynamics::BodyNode::Properties _bnProperties;
        _bnProperties.mName = shapeData.name;

        // create a node-joint pair according to the joint type
        if ( jointData.type == eJointType::FREE )
        {
            dynamics::FreeJoint::Properties _fjProperties;
            _fjProperties.mName = jointData.name;
            _fjProperties.mDampingCoefficients[0] = 0;
            _fjProperties.mDampingCoefficients[1] = 0;
            _fjProperties.mDampingCoefficients[2] = 0;
            _fjProperties.mDampingCoefficients[3] = 0;
            _fjProperties.mDampingCoefficients[4] = 0;
            _fjProperties.mDampingCoefficients[5] = 0;

            auto _joint_body_pair = m_dartSkeletonPtr->createJointAndBodyNodePair<dynamics::FreeJoint>(nullptr, _fjProperties, _bnProperties);

            m_dartJointPtr = _joint_body_pair.first;
            m_dartBodyNodePtr = _joint_body_pair.second;
        }
        else if ( jointData.type == eJointType::REVOLUTE )
        {
            // construct properties of the revolute joint
            dynamics::RevoluteJoint::Properties _rjProperties;

            _rjProperties.mName                     = jointData.name;
            _rjProperties.mAxis                     = toEigenVec3( jointData.axis );
            _rjProperties.mT_ParentBodyToJoint      = toEigenTransform( jointData.tfParentBody2Joint );
            _rjProperties.mT_ChildBodyToJoint       = toEigenTransform( jointData.tfThisBody2Joint );
            _rjProperties.mRestPositions[0]         = 0.0;
            _rjProperties.mSpringStiffnesses[0]     = 0.0;
            _rjProperties.mDampingCoefficients[0]   = 0.0;
            _rjProperties.mPositionLowerLimits[0]   = jointData.limits.x;
            _rjProperties.mPositionUpperLimits[0]   = jointData.limits.y;
            _rjProperties.mIsPositionLimitEnforced  = true;

            auto _joint_body_pair = m_dartSkeletonPtr->createJointAndBodyNodePair<dynamics::RevoluteJoint>(
                                                                _parentBodyNodePtr,
                                                                _rjProperties,
                                                                _bnProperties );

            m_dartJointPtr = _joint_body_pair.first;
            m_dartBodyNodePtr = _joint_body_pair.second;
        }
        else if ( jointData.type == eJointType::PRISMATIC )
        {
            // construct properties of the prismatic joint
            dynamics::PrismaticJoint::Properties _pjProperties;

            _pjProperties.mName                     = jointData.name;
            _pjProperties.mAxis                     = toEigenVec3( jointData.axis );
            _pjProperties.mT_ParentBodyToJoint      = toEigenTransform( jointData.tfParentBody2Joint );
            _pjProperties.mT_ChildBodyToJoint       = toEigenTransform( jointData.tfThisBody2Joint );
            _pjProperties.mRestPositions[0]         = 0.0;
            _pjProperties.mSpringStiffnesses[0]     = 0.0;
            _pjProperties.mDampingCoefficients[0]   = 0.0;

            auto _joint_body_pair = m_dartSkeletonPtr->createJointAndBodyNodePair<dynamics::PrismaticJoint>(
                                                                _parentBodyNodePtr,
                                                                _pjProperties,
                                                                _bnProperties );
            m_dartJointPtr = _joint_body_pair.first;
            m_dartBodyNodePtr = _joint_body_pair.second;
        }
        else if ( jointData.type == eJointType::BALL )
        {
            // construct properties of the ball|spherical joint
            dynamics::BallJoint::Properties _bjProperties;

            _bjProperties.mName                 = jointData.name;
            _bjProperties.mT_ParentBodyToJoint  = toEigenTransform( jointData.tfParentBody2Joint );
            _bjProperties.mT_ChildBodyToJoint   = toEigenTransform( jointData.tfThisBody2Joint );
            _bjProperties.mRestPositions        = Eigen::Vector3d::Constant( 0.0 );
            _bjProperties.mSpringStiffnesses    = Eigen::Vector3d( 0.0, 0.0, 0.0 );
            _bjProperties.mDampingCoefficients  = Eigen::Vector3d( 0.0, 0.0, 0.0 );

            auto _joint_body_pair = m_dartSkeletonPtr->createJointAndBodyNodePair<dynamics::BallJoint>(
                                                                _parentBodyNodePtr,
                                                                _bjProperties,
                                                                _bnProperties );
            m_dartJointPtr = _joint_body_pair.first;
            m_dartBodyNodePtr = _joint_body_pair.second;
        }
        else if ( jointData.type == eJointType::PLANAR )
        {
            // construct properties of the planar joint
            dynamics::PlanarJoint::Properties _pjProperties;

            _pjProperties.mName                 = jointData.name;
            _pjProperties.mT_ParentBodyToJoint  = toEigenTransform( jointData.tfParentBody2Joint );
            _pjProperties.mT_ChildBodyToJoint   = toEigenTransform( jointData.tfThisBody2Joint );
            _pjProperties.setZXPlane();

            auto _joint_body_pair = m_dartSkeletonPtr->createJointAndBodyNodePair<dynamics::PlanarJoint>(
                                                                _parentBodyNodePtr,
                                                                _pjProperties,
                                                                _bnProperties );
            m_dartJointPtr = _joint_body_pair.first;
            m_dartBodyNodePtr = _joint_body_pair.second;
        }
        else if ( jointData.type == eJointType::FIXED )
        {
            if ( _parentBodyNodePtr )
            {
                // construct properties of the fixed|weld joint
                dynamics::Joint::Properties _basicProperties;
                dynamics::WeldJoint::Properties _wjProperties( _basicProperties );

                _wjProperties.mName                 = jointData.name;
                _wjProperties.mT_ParentBodyToJoint  = toEigenTransform( jointData.tfParentBody2Joint );
                _wjProperties.mT_ChildBodyToJoint   = toEigenTransform( jointData.tfThisBody2Joint );

                auto _joint_body_pair = m_dartSkeletonPtr->createJointAndBodyNodePair<dynamics::WeldJoint>(
                                                                    _parentBodyNodePtr,
                                                                    _wjProperties,
                                                                    _bnProperties );
                m_dartJointPtr = _joint_body_pair.first;
                m_dartBodyNodePtr = _joint_body_pair.second;
            }
            else
            {
                // construct a static body
                dynamics::FreeJoint::Properties _wjProperties;
                _wjProperties.mDampingCoefficients[0] = 0;
                _wjProperties.mDampingCoefficients[1] = 0;
                _wjProperties.mDampingCoefficients[2] = 0;
                _wjProperties.mDampingCoefficients[3] = 0;
                _wjProperties.mDampingCoefficients[4] = 0;
                _wjProperties.mDampingCoefficients[5] = 0;

                auto _joint_body_pair = m_dartSkeletonPtr->createJointAndBodyNodePair<dynamics::WeldJoint>();

                m_dartJointPtr = _joint_body_pair.first;
                m_dartBodyNodePtr = _joint_body_pair.second;
            }
        }

        // construct the renderable to be used
        m_graphicsObj = createRenderableShape( shapeData );

        // add the collision shape to the body node
        if ( m_dartShapePtr )
        {
            m_dartShapeNodePtr = m_dartBodyNodePtr->createShapeNodeWith< 
                                                        dynamics::CollisionAspect, 
                                                        dynamics::DynamicsAspect >( m_dartShapePtr );
        }
        // m_dartShapeNodePtr->getDynamicsAspect()->setRestitutionCoeff( 0.6 );
        // m_dartShapeNodePtr->getDynamicsAspect()->setFrictionCoeff( 1.0 );


        if ( m_dartShapePtr )
        {
            // construct inertia properties
            dynamics::Inertia _bnInertia;

            // calculate the mass to be used for this body
            double _mass = computeMassFromShape( m_dartShapePtr );

            _bnInertia.setMass( _mass );
            _bnInertia.setMoment( m_dartShapePtr->computeInertia( _mass ) );

            m_dartBodyNodePtr->setInertia( _bnInertia );
        }
        // else
        // {
        //     _bnInertia.setMass( 0.001 );
        //     _bnInertia.setMoment( 0.000001, 0.000001, 0.000001, 0.000001, 0.000001, 0.000001 );
        // }
    }

    void SimBody::update()
    {
        if ( !m_dartShapeNodePtr )
            return;

        if ( !m_parent )
        {
            // in case of a free single body (or root) use the relative transform of the parent joint

            // convert joint rel-position (to world) into position to use
            Eigen::Isometry3d _tf = m_dartJointPtr->getRelativeTransform();
            m_worldPos = { (float)_tf.translation().x(), (float)_tf.translation().y(), (float)_tf.translation().z() };

            // convert joint rel-rotation (to world) into rotation to use
            Eigen::Quaterniond _quat( m_dartJointPtr->getRelativeTransform().rotation() );
            m_worldRot = tysoc::TMat3::fromQuaternion( { (float)_quat.x(), (float)_quat.y(), (float)_quat.z(), (float)_quat.w() } );

            // update the world-transform
            m_worldTransform = tysoc::TMat4::fromPositionAndRotation( m_worldPos, m_worldRot );
        }
        else
        {
            // otherwise, use the worldtransform of the bodynode

            // convert world-transform of bodynode into appropriate format
            m_worldTransform = fromEigenTransform( m_dartBodyNodePtr->getWorldTransform() );
            // and extract both position and rotation
            m_worldPos = m_worldTransform.getPosition();
            m_worldRot = m_worldTransform.getRotation();
        }
        
        if ( m_graphicsObj )
        {
            m_graphicsObj->position = { m_worldPos.x, m_worldPos.y, m_worldPos.z };

            for ( size_t i = 0; i < 3; i++ )
                for ( size_t j = 0; j < 3; j++ )
                    m_graphicsObj->rotation.set( i, j, m_worldRot.buff[i + 3 * j] );
        }

    }

    /***************************************************************************
    *                                                                          *
    *                             AGENT WRAPPER                                *
    *                                                                          *
    ***************************************************************************/

    SimAgent::SimAgent( const std::string& name,
                        std::shared_ptr<simulation::World> dartWorldPtr )
    {
        m_name = name;
        m_dartWorldPtr = dartWorldPtr;

        // construct a dart-skeleton for this agent, and let the user allocate
        // the remaining resources that it might need later (bodies, joints, etc.)
        m_dartSkeletonPtr = dart::dynamics::Skeleton::create( m_name );
    }

    SimBody* SimAgent::addBody( const std::string& name,
                                const std::string& parentName,
                                const ShapeData& shapeData,
                                const JointData& jointData )
    {
        SimBody* _parentSimBody = nullptr;
        // check if the parent exists, if not, just return nullptr and a ERROR msg
        if ( m_simBodiesMap.find( parentName ) != m_simBodiesMap.end() )
            _parentSimBody = m_simBodiesMap[ parentName ];

        auto _simBody = new SimBody( name, m_dartSkeletonPtr );

        _simBody->build( _parentSimBody, shapeData, jointData );

        // cache this body for later usage
        m_simBodies.push_back( _simBody );
        m_simBodiesMap[ _simBody->name() ] = _simBody;

        return _simBody;
    }

    void SimAgent::update()
    {
        // do nothing for now
    }

    /***************************************************************************
    *                                                                          *
    *                             BASE-APPLICATION                             *
    *                                                                          *
    ***************************************************************************/

    ITestApplication::ITestApplication()
    {
        m_dartWorldPtr = nullptr;

        m_graphicsApp = nullptr;
        m_graphicsScene = nullptr;

        m_isRunning = false;
        m_isTerminated = false;
    }

    ITestApplication::~ITestApplication()
    {
        for ( size_t i = 0; i < m_simBodies.size(); i++ )
            delete m_simBodies[i];

        m_simBodies.clear();
        m_simBodiesMap.clear();

        // @TODO: Check glfw's master branch, as mujoco's glfw seems be older
        // @TODO: Check why are we linking against glfw3 from mjc libs
    #if defined(__APPLE__) || defined(_WIN32)
        delete m_graphicsApp;
    #endif
        m_graphicsApp = nullptr;
        m_graphicsScene = nullptr;
    }

    void ITestApplication::init()
    {
        _initScenario();
        _initGraphics();
        _initPhysics();
        _onApplicationStart();

        togglePause();
    }

    void ITestApplication::_initScenario()
    {
        // delegate to virtual method
        _initScenarioInternal();
    }

    void ITestApplication::_initPhysics()
    {
        assert( m_graphicsApp != nullptr );
        assert( m_graphicsScene != nullptr );

        m_dartWorldPtr = dart::simulation::World::create();
        m_dartWorldPtr->getConstraintSolver()->setCollisionDetector( 
                                dart::collision::BulletCollisionDetector::create() );
        // m_dartWorldPtr->getConstraintSolver()->setLCPSolver(
        //                         dart::common::make_unique<dart::constraint::PGSLCPSolver>( m_dartWorldPtr->getTimeStep() ) );
        // m_dartWorldPtr->getConstraintSolver()->setLCPSolver( 
        //                         dart::common::make_unique<dart::constraint::PGSLCPSolver>( 
        //                             m_dartWorldPtr->getTimeStep() ) );
        // auto _boxedLcpConstraintSolver = reinterpret_cast< constraint::BoxedLcpConstraintSolver* >( m_dartWorldPtr->getConstraintSolver() );
        // _boxedLcpConstraintSolver->setBoxedLcpSolver( std::make_shared<constraint::PgsBoxedLcpSolver>() );
        m_dartWorldPtr->setTimeStep( 0.001 );

        m_isRunning = true;
        m_isTerminated = false;
    }

    void ITestApplication::_initGraphics()
    {
        auto _windowProperties = engine::CWindowProps();
        _windowProperties.width = 1024;
        _windowProperties.height = 768;
        _windowProperties.title = "resizable-application";
        _windowProperties.clearColor = { 0.6f, 0.659f, 0.690f, 1.0f };
        _windowProperties.resizable = true;

        auto _imguiProperties = engine::CImGuiProps();
        _imguiProperties.useDockingSpace = true;
        _imguiProperties.useDockingSpacePassthrough = true;
        _imguiProperties.useAutosaveLayout = false;
        _imguiProperties.fileLayout = std::string( TYSOC_PATH_RESOURCES ) + "app_gui_layout.ini";

        m_graphicsApp = new engine::CApplication( _windowProperties, _imguiProperties );
        m_graphicsScene = m_graphicsApp->scene();

        /* create some lights for the scene ***********************************************************/
        auto _dirlight = new engine::CDirectionalLight( "directional",
                                                        { 0.5f, 0.5f, 0.5f },
                                                        { 0.8f, 0.8f, 0.8f },
                                                        { 0.8f, 0.8f, 0.8f },
                                                        { -1.0f, -1.0f, -1.0f } );

        m_graphicsScene->addLight( std::unique_ptr< engine::CILight >( _dirlight ) );

        auto _pointlight = new engine::CPointLight( "point",
                                                    { 0.5f, 0.5f, 0.5f },
                                                    { 0.8f, 0.8f, 0.8f },
                                                    { 0.8f, 0.8f, 0.8f },
                                                    { 3.0f, 3.0f, 3.0f },
                                                    1.0f, 0.0f, 0.0f );

        m_graphicsScene->addLight( std::unique_ptr< engine::CILight >( _pointlight ) );
        /* create some cameras for the scene **********************************************************/
        auto _cameraProjData = engine::CCameraProjData();
        _cameraProjData.aspect = m_graphicsApp->window()->aspect();
        _cameraProjData.width = 10.0f * m_graphicsApp->window()->aspect();
        _cameraProjData.height = 10.0f;

        auto _orbitCamera = new engine::COrbitCamera( "orbit",
                                                      { 3.0f, 3.0f, 3.0f },
                                                      { 0.0f, 0.0f, 1.0f },
                                                      engine::eAxis::Z,
                                                      _cameraProjData,
                                                      m_graphicsApp->window()->width(),
                                                      m_graphicsApp->window()->height() );

        m_graphicsScene->addCamera( std::unique_ptr< engine::CICamera >( _orbitCamera ) );

        const float _cameraSensitivity  = 0.1f;
        const float _cameraSpeed        = 50.0f;
        const float _cameraMaxDelta     = 10.0f;
        
        auto _fpsCamera = new engine::CFpsCamera( "fps",
                                                  { 3.0f, 3.0f, 3.0f },
                                                  { 0.0f, 0.0f, 1.0f },
                                                  engine::eAxis::Z,
                                                  _cameraProjData,
                                                  _cameraSensitivity,
                                                  _cameraSpeed,
                                                  _cameraMaxDelta );

        m_graphicsScene->addCamera( std::unique_ptr< engine::CICamera >( _fpsCamera ) );
        /* add some effects like fog and a skybox *****************************************************/

        auto _skybox = new engine::CSkybox();
        _skybox->setCubemap( engine::CTextureManager::GetCachedTextureCube( "cloudtop" ) );

        m_graphicsScene->addSkybox( std::unique_ptr< engine::CSkybox >( _skybox ) );

        /**********************************************************************************************/

        m_graphicsApp->renderOptions().useSkybox = true;
        m_graphicsApp->renderOptions().useBlending = true;
        m_graphicsApp->renderOptions().useShadowMapping = true;
        m_graphicsApp->renderOptions().pcfCount = 0;
        m_graphicsApp->renderOptions().shadowMapRangeConfig.type = engine::eShadowRangeType::FIXED_USER;
        m_graphicsApp->renderOptions().shadowMapRangeConfig.worldUp = { 0.0f, 0.0f, 1.0f };
        m_graphicsApp->renderOptions().shadowMapRangeConfig.cameraPtr = _orbitCamera;
        m_graphicsApp->renderOptions().shadowMapRangeConfig.dirLightPtr = _dirlight;
    }

    void ITestApplication::reset()
    {
        // do some specific initialization
        _resetInternal();
    }

    void ITestApplication::step()
    {
        if ( m_isRunning && !m_isTerminated )
        {
            double _tstart = m_dartWorldPtr->getTime();
            while ( m_dartWorldPtr->getTime() - _tstart < 1. / 60. )
                m_dartWorldPtr->step();
        }

        // Update all body-wrappers
        for ( size_t i = 0; i < m_simBodies.size(); i++ )
        {
            if ( !m_simBodies[i] )
                continue;

            m_simBodies[i]->update();
        }

        // do some custom step functionality
        _stepInternal();

        if ( engine::CInputManager::CheckSingleKeyPress( ENGINE_KEY_G ) )
        {
            ENGINE_TRACE( "Toggling ui state" );
            m_graphicsApp->setGuiActive( !m_graphicsApp->guiActive() );
        }
        else if ( engine::CInputManager::CheckSingleKeyPress( ENGINE_KEY_U ) )
        {
            ENGINE_TRACE( "Toggling ui-utils state" );
            m_graphicsApp->setGuiUtilsActive( !m_graphicsApp->guiUtilsActive() );
        }
        else if ( engine::CInputManager::CheckSingleKeyPress( ENGINE_KEY_ESCAPE ) )
        {
            m_isTerminated = true;
        }
        else if ( engine::CInputManager::CheckSingleKeyPress( ENGINE_KEY_P ) )
        {
            togglePause();
        }
        else if ( engine::CInputManager::CheckSingleKeyPress( ENGINE_KEY_Q ) )
        {
            for ( size_t i = 0; i < m_simBodies.size(); i++ )
            {
                if ( !m_simBodies[i] )
                    continue;

                m_simBodies[i]->print();
            }
        }
        else if ( engine::CInputManager::CheckSingleKeyPress( ENGINE_KEY_R ) )
        {
            // reset functionality here
            std::cout << "INFO> requested reset of the simulation" << std::endl;
        }

        engine::CDebugDrawer::DrawLine( { 0.0f, 0.0f, 0.0f }, { 5.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } );
        engine::CDebugDrawer::DrawLine( { 0.0f, 0.0f, 0.0f }, { 0.0f, 5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } );
        engine::CDebugDrawer::DrawLine( { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, 1.0f } );

        m_graphicsApp->update();
        m_graphicsApp->begin();
        m_graphicsApp->render();
        m_graphicsApp->end();
    }

    void ITestApplication::_renderUi()
    {
        // render ui related to agents
        _renderUiAgents();

        // Call some custom render functionality
        _renderUiInternal();
    }

    void ITestApplication::_renderUiAgents()
    {
        // do nothing for now
    }

    void ITestApplication::togglePause()
    {
        m_isRunning = ( m_isRunning ) ? false : true;
    }

    SimBody* ITestApplication::createSingleBody( const ShapeData& shapeData, bool isFree )
    {
        auto _skeleton = dynamics::Skeleton::create( shapeData.name );
        
        // construct joint data for this guy
        JointData _jointData;
        if ( isFree )
        {
            _jointData.name = "free_" + shapeData.name;
            _jointData.type = eJointType::FREE;
        }
        else
        {
            _jointData.name = "fixed_" + shapeData.name;
            _jointData.type = eJointType::FIXED;
        }
        
        auto _simBody = new SimBody( shapeData.name, _skeleton );
        _simBody->build( nullptr, shapeData, _jointData );

        /**********  Create the qpos for the object's initial configuration ********/
        if ( isFree )
        {
            auto _tf = toEigenTransform( tysoc::TMat4( shapeData.localPos, shapeData.localRot ) );
            auto _qpos0 = dynamics::FreeJoint::convertToPositions( _tf );
            
            _skeleton->getJoint( 0 )->setPositions( _qpos0 );
        }
        else 
        {
            auto _tf = toEigenTransform( tysoc::TMat4( shapeData.localPos, shapeData.localRot) );
            _simBody->node()->getParentJoint()->setTransformFromParentBodyNode( _tf );
        }

        m_dartWorldPtr->addSkeleton( _skeleton );

        m_simBodies.push_back( _simBody );
        m_simBodiesMap[ _simBody->name() ] = _simBody;

        m_graphicsScene->addRenderable( std::unique_ptr< engine::CIRenderable >( _simBody->graphics() ) );

        return _simBody;
    }

    void ITestApplication::addSimAgent( SimAgent* simAgentPtr, tysoc::TVec3& position, tysoc::TMat3& rotation )
    {
        m_dartWorldPtr->addSkeleton( simAgentPtr->skeleton() );

        m_simAgents.push_back( simAgentPtr );
        m_simAgentsMap[ simAgentPtr->name() ] = simAgentPtr;

        auto _bodies = simAgentPtr->bodies();
        for ( size_t i = 0; i < _bodies.size(); i++ )
        {
            assert( _bodies[i] ); // ensure that the body exists

            m_simBodies.push_back( _bodies[i] );
            m_simBodiesMap[ _bodies[i]->name() ] = _bodies[i];

            if ( _bodies[i]->graphics() )
                m_graphicsScene->addRenderable( std::unique_ptr< engine::CIRenderable >(_bodies[i]->graphics() ) );
        }

        // grab root and place it in the starting position
        auto _rootSimBodyPtr = _bodies.front();

        if ( _rootSimBodyPtr->joint()->getType() == dynamics::FreeJoint::getStaticType() )
        {
            auto _tf = toEigenTransform( tysoc::TMat4( position, rotation ) );
            auto _qpos0 = dynamics::FreeJoint::convertToPositions( _tf );

            simAgentPtr->skeleton()->getJoint( 0 )->setPositions( _qpos0 );
        }
        else if ( _rootSimBodyPtr->joint()->getType() == dynamics::PlanarJoint::getStaticType() )
        {
            auto _qpos0 = Eigen::Vector3d( position.z, position.x, TYSOC_PI / 90. );

            simAgentPtr->skeleton()->getJoint( 0 )->setPositions( _qpos0 );
        }
    }

    SimBody* ITestApplication::getBody( const std::string& name )
    {
        if ( m_simBodiesMap.find( name ) != m_simBodiesMap.end() )
            return m_simBodiesMap[name];

        std::cout << "ERROR> body with name: " << name << " not found" << std::endl;
        return nullptr;
    }

}
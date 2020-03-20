
#include <primitives/loco_single_body_adapter_dart.h>

namespace loco {
namespace dartsim {

    TDartSingleBodyAdapter::TDartSingleBodyAdapter( TSingleBody* body_ref )
        : TISingleBodyAdapter( body_ref )
    {
        LOCO_CORE_ASSERT( body_ref, "TDartSingleBodyAdapter >>> expected non-null body-obj reference" );

        m_DartSkeleton = nullptr;
        m_DartBodyNodeRef = nullptr;
        m_DartJointRef = nullptr;
        m_DartWorldRef = nullptr;
    }

    TDartSingleBodyAdapter::~TDartSingleBodyAdapter()
    {
        if ( m_BodyRef )
            m_BodyRef->DetachSim();
        m_BodyRef = nullptr;

        m_DartSkeleton = nullptr;
        m_DartBodyNodeRef = nullptr;
        m_DartJointRef = nullptr;
        m_DartWorldRef = nullptr;
    }

    void TDartSingleBodyAdapter::Build()
    {
        m_DartSkeleton = dart::dynamics::Skeleton::create( m_BodyRef->name() );
        const auto dyntype = m_BodyRef->dyntype();
        if ( dyntype == eDynamicsType::STATIC )
        {
            dart::dynamics::BodyNode::Properties body_properties;
            body_properties.mName = m_BodyRef->name();
            dart::dynamics::WeldJoint::Properties joint_properties;
            joint_properties.mName = m_BodyRef->name() + "_weldjoint";

            auto joint_bodynode_pair = m_DartSkeleton->createJointAndBodyNodePair<dart::dynamics::WeldJoint>(
                                                            nullptr, joint_properties, body_properties );
            m_DartJointRef = joint_bodynode_pair.first;
            m_DartBodyNodeRef = joint_bodynode_pair.second;
        }
        else
        {
            dart::dynamics::BodyNode::Properties body_properties;
            body_properties.mName = m_BodyRef->name();
            dart::dynamics::FreeJoint::Properties joint_properties;
            joint_properties.mName = m_BodyRef->name() + "_freejoint";

            auto joint_bodynode_pair = m_DartSkeleton->createJointAndBodyNodePair<dart::dynamics::FreeJoint>(
                                                            nullptr, joint_properties, body_properties );
            m_DartJointRef = joint_bodynode_pair.first;
            m_DartBodyNodeRef = joint_bodynode_pair.second;
        }

        if ( auto collider = m_BodyRef->collider() )
        {
            if ( auto collider_adapter = static_cast<TDartSingleBodyColliderAdapter*>( collider->collider_adapter() ) )
            {
                collider_adapter->Build();
                auto& dart_collision_shape = collider_adapter->collision_shape();
                auto shape_node = m_DartBodyNodeRef->createShapeNodeWith<
                                                        dart::dynamics::CollisionAspect,
                                                        dart::dynamics::DynamicsAspect>( dart_collision_shape );
                collider_adapter->SetDartShapeNode( shape_node );
                if ( dyntype == eDynamicsType::DYNAMIC )
                {
                    dart::dynamics::Inertia body_inertia;
                    const auto& inertia_data = m_BodyRef->data().inertia;

                    if ( inertia_data.mass > 0.0f )
                        body_inertia.setMass( inertia_data.mass );
                    else
                        body_inertia.setMass( dart_collision_shape->getVolume() * collider->data().density );

                    if ( ( inertia_data.ixx > 0.0f ) && ( inertia_data.iyy > 0.0f ) && ( inertia_data.izz > 0.0f ) && 
                         ( inertia_data.ixy >= 0.0f ) && ( inertia_data.ixz >= 0.0f ) && ( inertia_data.iyz >= 0.0f ) )
                        body_inertia.setMoment( inertia_data.ixx, inertia_data.iyy, inertia_data.izz,
                                                inertia_data.ixy, inertia_data.ixz, inertia_data.iyz );
                    else
                        body_inertia.setMoment( dart_collision_shape->computeInertia( body_inertia.getMass() ) );

                    m_DartBodyNodeRef->setInertia( body_inertia );
                }
            }
        }

        SetTransform( m_BodyRef->tf0() );
    }

    void TDartSingleBodyAdapter::Initialize()
    {
        LOCO_CORE_ASSERT( m_DartWorldRef, "TDartSingleBodyAdapter::Initialize >>> body {0} must have \
                          a valid dart-world reference", m_BodyRef->name() );
        LOCO_CORE_ASSERT( m_DartSkeleton, "TDartSingleBodyAdapter::Initialize >>> body {0} must have \
                          a valid dart-rigid-body. Perhaps missing call to ->Build()?", m_BodyRef->name() );

        m_DartWorldRef->addSkeleton( m_DartSkeleton );
    }

    void TDartSingleBodyAdapter::Reset()
    {
        SetTransform( m_BodyRef->tf0() );

        if ( m_BodyRef->dyntype() == eDynamicsType::DYNAMIC )
        {
            SetLinearVelocity( { 0.0, 0.0, 0.0 } );
            SetAngularVelocity( { 0.0, 0.0, 0.0 } );
        }
    }

    void TDartSingleBodyAdapter::OnDetach()
    {
        m_Detached = true;
        m_BodyRef = nullptr;
    }

    void TDartSingleBodyAdapter::SetTransform( const TMat4& transform )
    {
        LOCO_CORE_ASSERT( m_DartJointRef, "TDartSingleBodyAdapter::SetTransform >>> body {0} must have \
                          a valid dart-joint to set its transform. Perhaps missing call to ->Build()", m_BodyRef->name() );

        auto dart_tf = mat4_to_eigen_tf( transform );
        if ( m_BodyRef->dyntype() == eDynamicsType::DYNAMIC )
            static_cast<dart::dynamics::FreeJoint*>( m_DartJointRef )->setTransform( dart_tf );
        else
            m_DartJointRef->setTransformFromParentBodyNode( dart_tf );
    }

    void TDartSingleBodyAdapter::SetLinearVelocity( const TVec3& linear_vel )
    {
        LOCO_CORE_ASSERT( m_DartJointRef, "TDartSingleBodyAdapter::SetLinearVelocity >>> body {0} must have \
                          a valid dart-joint to set its linear velocity. Perhaps missing call to ->Build()", m_BodyRef->name() )
        LOCO_CORE_ASSERT( m_DartBodyNodeRef, "TDartSingleBodyAdapter::SetLinearVelocity >>> body {0} must have \
                          a valid dart-bodynode to set its linear velocity. Perhaps missing call to ->Build()", m_BodyRef->name() )

        if ( m_DartJointRef->getNumDofs() != 6 )
        {
            LOCO_CORE_WARN( "TDartSingleBodyAdapter::SetLinearVelocity >>> body {0} should be a free body, with \
                             6 degrees of freedom. Current number of dofs is {1}", m_BodyRef->name(), m_DartJointRef->getNumDofs() );
            return;
        }

        Eigen::Isometry3d center_tf( Eigen::Isometry3d::Identity() );
        center_tf.translation() = m_DartSkeleton->getCOM();

        dart::dynamics::SimpleFrame center( dart::dynamics::Frame::World(), "center", center_tf );
        center.setClassicDerivatives( vec3_to_eigen( linear_vel ), m_DartBodyNodeRef->getAngularVelocity() );

        dart::dynamics::SimpleFrame ref( &center, "root_reference" );
        ref.setRelativeTransform( m_DartBodyNodeRef->getTransform( &center ) );

        m_DartJointRef->setVelocities( ref.getSpatialVelocity() );
    }

    void TDartSingleBodyAdapter::SetAngularVelocity( const TVec3& angular_vel )
    {
        LOCO_CORE_ASSERT( m_DartJointRef, "TDartSingleBodyAdapter::SetAngularVelocity >>> body {0} must have \
                          a valid dart-joint to set its angular velocity. Perhaps missing call to ->Build()", m_BodyRef->name() )
        LOCO_CORE_ASSERT( m_DartBodyNodeRef, "TDartSingleBodyAdapter::SetAngularVelocity >>> body {0} must have \
                          a valid dart-bodynode to set its angular velocity. Perhaps missing call to ->Build()", m_BodyRef->name() )

        if ( m_DartJointRef->getNumDofs() != 6 )
        {
            LOCO_CORE_WARN( "TDartSingleBodyAdapter::SetAngularVelocity >>> body {0} should be a free body, with \
                             6 degrees of freedom. Current number of dofs is {1}", m_BodyRef->name(), m_DartJointRef->getNumDofs() );
            return;
        }

        Eigen::Isometry3d center_tf( Eigen::Isometry3d::Identity() );
        center_tf.translation() = m_DartSkeleton->getCOM();

        dart::dynamics::SimpleFrame center( dart::dynamics::Frame::World(), "center", center_tf );
        center.setClassicDerivatives( m_DartBodyNodeRef->getLinearVelocity(), vec3_to_eigen( angular_vel ) );

        dart::dynamics::SimpleFrame ref( &center, "root_reference" );
        ref.setRelativeTransform( m_DartBodyNodeRef->getTransform( &center ) );

        m_DartJointRef->setVelocities( ref.getSpatialVelocity() );
    }

    void TDartSingleBodyAdapter::SetForceCOM( const TVec3& force_com )
    {
        LOCO_CORE_ASSERT( m_DartBodyNodeRef, "TDartSingleBodyAdapter::SetForceCOM >>> body {0} must have \
                          a valid dart-bodynode to set a force @ com. Perhaps missing call to ->Build()", m_BodyRef->name() );

        m_DartBodyNodeRef->setExtForce( vec3_to_eigen( force_com ) );
    }

    void TDartSingleBodyAdapter::SetTorqueCOM( const TVec3& torque_com )
    {
        LOCO_CORE_ASSERT( m_DartBodyNodeRef, "TDartSingleBodyAdapter::SetTorqueCOM >>> body {0} must have \
                          a valid dart-bodynode to set a torque @ com. Perhaps missing call to ->Build()", m_BodyRef->name() );

        m_DartBodyNodeRef->setExtTorque( vec3_to_eigen( torque_com ) );
    }

    void TDartSingleBodyAdapter::GetTransform( TMat4& dst_transform )
    {
        LOCO_CORE_ASSERT( m_DartBodyNodeRef, "TDartSingleBodyAdapter::GetTransform >>> body {0} must have \
                          a valid dart-bodynode to get its transform. Perhaps missing call to ->Build()", m_BodyRef->name() );

        dst_transform = mat4_from_eigen_tf( m_DartBodyNodeRef->getTransform() );
    }

    void TDartSingleBodyAdapter::GetLinearVelocity( TVec3& dst_linear_vel )
    {
        LOCO_CORE_ASSERT( m_DartBodyNodeRef, "TDartSingleBodyAdapter::GetLinearVelocity >>> body {0} must have \
                          a valid dart-bodynode to get its linear velocity. Perhaps missing call to ->Build()", m_BodyRef->name() );

        dst_linear_vel = vec3_from_eigen( m_DartBodyNodeRef->getLinearVelocity() );
    }

    void TDartSingleBodyAdapter::GetAngularVelocity( TVec3& dst_angular_vel )
    {
        LOCO_CORE_ASSERT( m_DartBodyNodeRef, "TDartSingleBodyAdapter::GetAngularVelocity >>> body {0} must have \
                          a valid dart-bodynode to get its angular velocity. Perhaps missing call to ->Build()", m_BodyRef->name() );

        dst_angular_vel = vec3_from_eigen( m_DartBodyNodeRef->getAngularVelocity() );
    }

}}
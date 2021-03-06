
#include <loco.h>
#include <gtest/gtest.h>

#include <loco_simulation_dart.h>
#include <primitives/loco_single_body_adapter_dart.h>

struct BuildGroup
{
    std::unique_ptr<loco::TSingleBody> body;
    std::unique_ptr<loco::dartsim::TDartSingleBodyAdapter> body_adapter;
    std::unique_ptr<loco::dartsim::TDartSingleBodyColliderAdapter> collision_adapter;
};

BuildGroup build_body( const std::string& name,
                       const loco::eShapeType& shape,
                       const loco::TVec3& size,
                       const loco::eDynamicsType& dyntype,
                       const loco::TVec3& position,
                       const loco::TVec3& roteuler,
                       const loco::TInertialData inertia = loco::TInertialData(),
                       const std::string& mesh_filepath = "" )
{
    auto col_data = loco::TCollisionData();
    col_data.type = shape;
    col_data.size = size;
    col_data.mesh_data.filename = mesh_filepath;
    auto vis_data = loco::TVisualData();
    vis_data.type = shape;
    vis_data.size = size;
    vis_data.mesh_data.filename = mesh_filepath;
    auto body_data = loco::TBodyData();
    body_data.collision = col_data;
    body_data.visual = vis_data;
    body_data.inertia = inertia;
    body_data.dyntype = dyntype;

    auto body_obj = std::make_unique<loco::TSingleBody>( name, body_data, position, tinymath::rotation( roteuler ) );
    auto body_adapter = std::make_unique<loco::dartsim::TDartSingleBodyAdapter>( body_obj.get() );
    auto collision_ref = body_obj->collider();
    auto collision_adapter = std::make_unique<loco::dartsim::TDartSingleBodyColliderAdapter>( collision_ref );
    collision_ref->SetColliderAdapter( collision_adapter.get() );
    body_adapter->Build();

    return { std::move( body_obj ), std::move( body_adapter ), std::move( collision_adapter ) };
}

TEST( TestLocoDartSingleBodyAdapter, TestLocoDartSingleBodyAdapterBuild )
{
    loco::InitUtils();

    std::vector<std::string> vec_names = { "floor",
                                           "boxy",
                                           "box_obstacle",
                                           "monkey_head",
                                           "heavy_sphere" };
    std::vector<loco::TVec3> vec_sizes = { { 10.0, 10.0, 1.0 },
                                           { 0.1, 0.2, 0.3 },
                                           { 0.1, 0.2, 0.3 },
                                           { 0.2, 0.2, 0.2 },
                                           { 0.1, 0.1, 0.1 } };
    std::vector<loco::eShapeType> vec_shapes = { loco::eShapeType::PLANE,
                                                 loco::eShapeType::BOX,
                                                 loco::eShapeType::BOX,
                                                 loco::eShapeType::MESH,
                                                 loco::eShapeType::SPHERE };
    std::vector<loco::eDynamicsType> vec_dyntypes = { loco::eDynamicsType::STATIC,
                                                      loco::eDynamicsType::DYNAMIC,
                                                      loco::eDynamicsType::STATIC,
                                                      loco::eDynamicsType::DYNAMIC,
                                                      loco::eDynamicsType::DYNAMIC };
    std::vector<loco::TVec3> vec_positions = { { 0.0, 0.0, 0.0 },
                                               { 0.0, 0.0, 1.0 },
                                               { 0.0, 1.0, 1.0 },
                                               { 1.0, 0.0, 1.0 },
                                               { 1.0, 1.0, 1.0 } };
    std::vector<loco::TVec3> vec_eulers = { { 0.0, 0.0, 0.0 },
                                            { 0.2 * loco::PI, 0.2 * loco::PI, 0.2 * loco::PI },
                                            { 0.3 * loco::PI, 0.3 * loco::PI, 0.3 * loco::PI },
                                            { 0.4 * loco::PI, 0.4 * loco::PI, 0.4 * loco::PI },
                                            { 0.5 * loco::PI, 0.5 * loco::PI, 0.5 * loco::PI } };
    std::vector<double> vec_expected_masses = { 1.0, // static-dyntype
                                                0.1 * 0.2 * 0.3 * loco::DEFAULT_DENSITY,
                                                1.0, // static-dyntype
                                                0.0, // don't check (requires aabb)
                                                (4. / 3.) * loco::PI * 0.1 * 0.1 * 0.1 * loco::DEFAULT_DENSITY };
    const std::string monkey_filepath = loco::PATH_RESOURCES + "meshes/monkey.stl";
    auto scenario = std::make_unique<loco::TScenario>();
    for ( ssize_t i = 0; i < 5; i++ )
    {
        auto body_group = build_body( vec_names[i], vec_shapes[i], vec_sizes[i], vec_dyntypes[i],
                                      vec_positions[i], vec_eulers[i], loco::TInertialData(),
                                      ( vec_shapes[i] == loco::eShapeType::MESH ? monkey_filepath : "" ) );
        auto dart_body_node = body_group.body_adapter->body_node();
        ASSERT_TRUE( dart_body_node != nullptr );

        const auto tf = loco::dartsim::mat4_from_eigen_tf( dart_body_node->getTransform() );
        const auto expected_tf = body_group.body->tf();
        EXPECT_TRUE( tinymath::allclose( tf, expected_tf, 1e-2f ) );

        if ( vec_shapes[i] != loco::eShapeType::MESH )
        {
            const auto mass = dart_body_node->getMass();
            const auto expected_mass = vec_expected_masses[i];
            LOCO_CORE_TRACE( "shape: {0}, mass: {1}, expected_mass: {2}", loco::ToString( vec_shapes[i] ), mass, expected_mass );
            EXPECT_TRUE( std::abs( mass - expected_mass ) < 1e-5 );
        }

        scenario->AddSingleBody( std::move( body_group.body ) );
    }

    auto simulation = std::make_unique<loco::dartsim::TDartSimulation>( scenario.get() );
    simulation->Initialize();
}


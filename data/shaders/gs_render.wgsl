
struct CameraData {
    view_projection : mat4x4f,
    eye : vec3f,
    exposure : f32,
    right_controller_position : vec3f,
    ibl_intensity : f32
};

@group(0) @binding(0) var<uniform> model : mat4x4f;
#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) rotation: vec4<f32>,
    @location(2) color: vec3<f32>,
    @location(3) scale: vec3<f32>,
    @location(4) opacity: f32
};

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) color: vec4<f32>,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.color = vec4<f32>(in.color, in.opacity);
    let world_pos : vec4f = model * vec4<f32>(in.position, 1.0);
    out.clip_position = camera_data.view_projection * world_pos;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return in.color * camera_data.exposure;
}
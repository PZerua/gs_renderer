@binding(0) @group(0) var<storage, read> input :array<u32>;
@binding(1) @group(0) var<storage, read> inputId :array<u32>;
@binding(2) @group(0) var<storage, read> temp :array<vec4<u32>>;
@binding(3) @group(0) var<storage, read> sums: array<u32>;
@binding(4) @group(0) var<uniform> sumSize: u32;
@binding(5) @group(0) var<storage, read_write> output :array<u32>;
@binding(6) @group(0) var<storage, read_write> outputId :array<u32>;

const n : u32 = 512;

@binding(0) @group(1) var<uniform> radixMaskId:u32;

@compute @workgroup_size(256)
fn compute(@builtin(global_invocation_id) GlobalInvocationID : vec3<u32>,
    @builtin(local_invocation_id) LocalInvocationID: vec3<u32>,
    @builtin(workgroup_id) WorkgroupID: vec3<u32>) 
{
    var thid:u32 = LocalInvocationID.x;
    var globalThid:u32 = GlobalInvocationID.x;
    var mask:u32 = u32(3) << (radixMaskId << 1);
    
    var count0beforeCurrentWorkgroup:u32 = 0;
    var count1beforeCurrentWorkgroup:u32 = 0;
    var count2beforeCurrentWorkgroup:u32 = 0;
    var count3beforeCurrentWorkgroup:u32 = 0;

    if (WorkgroupID.x > 0) {
        count0beforeCurrentWorkgroup = sums[(WorkgroupID.x-1) * 4];
        count1beforeCurrentWorkgroup = sums[(WorkgroupID.x-1) * 4+1];
        count2beforeCurrentWorkgroup = sums[(WorkgroupID.x-1) * 4+2];
        count3beforeCurrentWorkgroup =  sums[(WorkgroupID.x-1) * 4+3];
    }

    var count0overall:u32 = sums[(sumSize-1)*4];
    var count1overall:u32 = sums[(sumSize-1)*4+1];
    var count2overall:u32 = sums[(sumSize-1)*4+2];
    var count3overall:u32 = sums[(sumSize-1)*4+3];

    if (thid < (n>>1)){
        var val:u32 = (input[2*globalThid] & mask) >> (radixMaskId << 1);

        var id:u32 = 0;

        if (val == 0) {
            id += temp[2*globalThid].x +  count0beforeCurrentWorkgroup;
        }
        else if (val == 1) {
            id +=count0overall;
            id += temp[2*globalThid].y + count1beforeCurrentWorkgroup;
        }
        else if (val == 2) {
            id += count0overall;
            id += count1overall;
            id += temp[2*globalThid].z +  count2beforeCurrentWorkgroup;
        }
        else if (val == 3) {
            id +=count0overall;
            id +=count1overall;
            id +=count2overall;
            id += temp[2*globalThid].w + count3beforeCurrentWorkgroup;
        }

        output[id] = input[2*globalThid]; 
        outputId[id] = inputId[2*globalThid];
        //output[2*globalThid] = id;

        id = 0;

        val = (input[2*globalThid+1] & mask) >> (radixMaskId << 1);

        if (val == 0) {
            id += temp[2*globalThid+1].x +  count0beforeCurrentWorkgroup;
        }
        else if (val == 1) {
            id +=count0overall;
            id += temp[2*globalThid+1].y + count1beforeCurrentWorkgroup ;
        }
        else if (val == 2) {
            id += count0overall;
            id += count1overall;
            id += temp[2*globalThid+1].z + count2beforeCurrentWorkgroup;
        }
        else if (val == 3) {
            id +=count0overall;
            id +=count1overall;
            id +=count2overall;
            id += temp[2*globalThid+1].w + count3beforeCurrentWorkgroup;
        }

        output[id] = input[2*globalThid+1];
        outputId[id] = inputId[2*globalThid+1];
        //output[2*globalThid+1] = id;
    }
}
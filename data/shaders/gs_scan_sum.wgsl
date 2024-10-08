@group(0) @binding(1) var<storage, read> input : array<u32>;
@group(0) @binding(3) var<storage, read_write> output : array<u32>;
@group(0) @binding(4) var<uniform> n : u32;

const bank_size : u32 = 32;
var<workgroup> temp: array<vec4<u32>, 532>; //workgroup array must have a fixed size;

fn bank_conflict_free_idx( idx:u32) -> u32 {
    var chunk_id:u32 = idx / bank_size;
    return idx + chunk_id;
}

/*fn bank_conflict_free_idx( idx:u32) -> u32 { // fake
    return idx;
}*/

@compute @workgroup_size(256)
fn compute(@builtin(global_invocation_id) GlobalInvocationID : vec3<u32>,
    @builtin(local_invocation_id) LocalInvocationID: vec3<u32>,
    @builtin(workgroup_id) WorkgroupID: vec3<u32>) 
{
    var thid:u32 = LocalInvocationID.x;
    var globalThid:u32 = GlobalInvocationID.x;
    if (thid < (n>>1)){
        temp[bank_conflict_free_idx(2*thid)] = vec4<u32>( input[(2*globalThid)*4],
        input[(2*globalThid)*4+1], input[(2*globalThid)*4+2],  input[(2*globalThid)*4+3]);

        temp[bank_conflict_free_idx(2*thid+1)] = vec4<u32>( input[(2*globalThid+1)*4],
        input[(2*globalThid+1)*4+1], input[(2*globalThid+1)*4+2], input[(2*globalThid+1)*4+3]);
    }

    workgroupBarrier();
    var offset:u32 = 1;

    for (var d:u32 = n>>1; d > 0; d >>= 1)
    { 
        if (thid < d)    
        {
            var ai:u32 = offset*(2*thid+1)-1;     
            var bi:u32 = offset*(2*thid+2)-1;  
            temp[bank_conflict_free_idx(bi)] += temp[bank_conflict_free_idx(ai)];    
        }    
        offset *= 2; 

        workgroupBarrier();   
    }

    if (thid == 0) 
    { 
        temp[bank_conflict_free_idx(n - 1)]= vec4<u32>(0,0,0,0); 
    } // clear the last element  
    workgroupBarrier();      

    for (var d:u32 = 1; d < n; d *= 2) // traverse down tree & build scan 
    {      
        offset >>= 1;      
        if (thid < d)      
        { 
            var ai:u32 = offset*(2*thid+1)-1;     
            var bi:u32 = offset*(2*thid+2)-1; 
            var t:vec4<u32> = temp[bank_conflict_free_idx(ai)]; 
            temp[bank_conflict_free_idx(ai)] = temp[bank_conflict_free_idx(bi)]; 
            temp[bank_conflict_free_idx(bi)] += t;       
        } 
        workgroupBarrier();      
    }
    
    if (thid < (n>>1)){
        output[(2*globalThid)*4] =   temp[bank_conflict_free_idx(2*thid)][0] +  input[(2*globalThid)*4]; 
        output[(2*globalThid)*4+1] = temp[bank_conflict_free_idx(2*thid)][1] +  input[(2*globalThid)*4+1]; 
        output[(2*globalThid)*4+2] = temp[bank_conflict_free_idx(2*thid)][2] +  input[(2*globalThid)*4+2]; 
        output[(2*globalThid)*4+3] = temp[bank_conflict_free_idx(2*thid)][3] +  input[(2*globalThid)*4+3]; 

        output[(2*globalThid+1)*4] =   temp[bank_conflict_free_idx(2*thid+1)][0] + input[(2*globalThid+1)*4]; 
        output[(2*globalThid+1)*4+1] = temp[bank_conflict_free_idx(2*thid+1)][1] + input[(2*globalThid+1)*4+1]; 
        output[(2*globalThid+1)*4+2] = temp[bank_conflict_free_idx(2*thid+1)][2] + input[(2*globalThid+1)*4+2];  
        output[(2*globalThid+1)*4+3] = temp[bank_conflict_free_idx(2*thid+1)][3] + input[(2*globalThid+1)*4+3]; 
    }
}
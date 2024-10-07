@binding(0) @group(0) var<storage, read> input :array<u32>;
@binding(1) @group(0) var<storage, read_write> output :array<vec4<u32>>;
@binding(2) @group(0) var<storage, read_write> sums: array<u32>;

@binding(0) @group(1) var<uniform> radixMaskId:u32;

const bank_size : u32 = 32;
const n : u32 = 512;
var<workgroup> temp0: array<u32, 532>;
var<workgroup> temp1: array<u32, 532>;
var<workgroup> temp2: array<u32, 532>;
var<workgroup> temp3: array<u32, 532>;

fn bank_conflict_free_idx( idx : u32) -> u32 {
    var chunk_id : u32 = idx / bank_size;
    return idx + chunk_id;
}

@compute @workgroup_size(256)
fn compute(@builtin(global_invocation_id) GlobalInvocationID : vec3<u32>,
    @builtin(local_invocation_id) LocalInvocationID: vec3<u32>,
    @builtin(workgroup_id) WorkgroupID: vec3<u32>) {
        var thid:u32 = LocalInvocationID.x;
        var globalThid:u32 = GlobalInvocationID.x;
        var mask:u32 = u32(3) << (radixMaskId << 1);

        if (thid < (n>>1)){

            var val:u32 = (input[2*globalThid] & mask) >> (radixMaskId << 1);

            if (val == 0) {
                temp0[bank_conflict_free_idx(2*thid)] = 1;
            }
            else if (val == 1) {
                temp1[bank_conflict_free_idx(2*thid)] = 1;
            }
            else if (val == 2) {
                temp2[bank_conflict_free_idx(2*thid)] = 1;
            }
            else if (val == 3) {
                temp3[bank_conflict_free_idx(2*thid)] = 1;
            }

            val = (input[2*globalThid+1] & mask) >> (radixMaskId << 1);

            if (val == 0) {
                temp0[bank_conflict_free_idx(2*thid+1)] = 1;
            }
            else if (val == 1) {
                temp1[bank_conflict_free_idx(2*thid+1)] = 1;
            }
            else if (val == 2) {
                temp2[bank_conflict_free_idx(2*thid+1)] = 1;
            }
            else if (val == 3) {
                temp3[bank_conflict_free_idx(2*thid+1)] = 1;
            }
        }
        workgroupBarrier();

        var offset:u32 = 1;

        for (var d:u32 = n>>1; d > 0; d >>= 1)
        { 
            if (thid < d)    
            {
                var ai:u32 = offset*(2*thid+1)-1;     
                var bi:u32 = offset*(2*thid+2)-1;  
                temp0[bank_conflict_free_idx(bi)] += temp0[bank_conflict_free_idx(ai)]; 
                temp1[bank_conflict_free_idx(bi)] += temp1[bank_conflict_free_idx(ai)];   
                temp2[bank_conflict_free_idx(bi)] += temp2[bank_conflict_free_idx(ai)];    
                temp3[bank_conflict_free_idx(bi)] += temp3[bank_conflict_free_idx(ai)];    
            }    
            offset *= 2; 

            workgroupBarrier();   
        }

        if (thid == 0) 
        { 
            temp0[bank_conflict_free_idx(n - 1)] = 0; 
            temp1[bank_conflict_free_idx(n - 1)] = 0; 
            temp2[bank_conflict_free_idx(n - 1)] = 0; 
            temp3[bank_conflict_free_idx(n - 1)] = 0; 
        }
        workgroupBarrier();      

        for (var d:u32 = 1; d < n; d *= 2) // traverse down tree & build scan 
        {      
            offset >>= 1;      
            if (thid < d)      
            { 
                var ai:u32 = offset*(2*thid+1)-1;     
                var bi:u32 = offset*(2*thid+2)-1; 
                var t:u32 = temp0[bank_conflict_free_idx(ai)]; 
                temp0[bank_conflict_free_idx(ai)] = temp0[bank_conflict_free_idx(bi)]; 
                temp0[bank_conflict_free_idx(bi)] += t;     
                
                t = temp1[bank_conflict_free_idx(ai)]; 
                temp1[bank_conflict_free_idx(ai)] = temp1[bank_conflict_free_idx(bi)]; 
                temp1[bank_conflict_free_idx(bi)] += t;    
                
                t = temp2[bank_conflict_free_idx(ai)]; 
                temp2[bank_conflict_free_idx(ai)] = temp2[bank_conflict_free_idx(bi)]; 
                temp2[bank_conflict_free_idx(bi)] += t;       

                t = temp3[bank_conflict_free_idx(ai)]; 
                temp3[bank_conflict_free_idx(ai)] = temp3[bank_conflict_free_idx(bi)]; 
                temp3[bank_conflict_free_idx(bi)] += t;       
            } 
            workgroupBarrier();      
        }

        var count0:u32 = temp0[bank_conflict_free_idx(2*255)] ;
        var count1:u32 = temp1[bank_conflict_free_idx(2*255)] ;
        var count2:u32 = temp2[bank_conflict_free_idx(2*255)] ;
        var count3:u32 = temp3[bank_conflict_free_idx(2*255)] ;

        var last:u32 =  (input[2*((WorkgroupID.x+1) * 256-1)] & mask)  >> (radixMaskId << 1); 
        switch(last) {
            case 0: {count0 += 1;}
            case 1: {count1 += 1;}
            case 2: {count2 += 1;}
            case 3: {count3 += 1;}
            default {}
        }

        last =  (input[2*((WorkgroupID.x+1) * 256-1)+1] & mask)  >> (radixMaskId << 1); 
        switch(last) {
            case 0: {count0 += 1;}
            case 1: {count1 += 1;}
            case 2: {count2 += 1;}
            case 3: {count3 += 1;}
            default {}
        }

        if (thid == 0) {
            sums[WorkgroupID.x * 4] = count0;
            sums[WorkgroupID.x * 4+1] = count1;
            sums[WorkgroupID.x * 4+2] = count2;
            sums[WorkgroupID.x * 4+3] = count3;
        }

        if (thid < (n>>1)){
            output[2*globalThid].x = temp0[bank_conflict_free_idx(2*thid)]; 
            output[2*globalThid+1].x = temp0[bank_conflict_free_idx(2*thid+1)]; 

            output[2*globalThid].y = temp1[bank_conflict_free_idx(2*thid)]; 
            output[2*globalThid+1].y = temp1[bank_conflict_free_idx(2*thid+1)]; 

            output[2*globalThid].z = temp2[bank_conflict_free_idx(2*thid)]; 
            output[2*globalThid+1].z = temp2[bank_conflict_free_idx(2*thid+1)]; 

            output[2*globalThid].w = temp3[bank_conflict_free_idx(2*thid)]; 
            output[2*globalThid+1].w = temp3[bank_conflict_free_idx(2*thid+1)]; 
        }
}
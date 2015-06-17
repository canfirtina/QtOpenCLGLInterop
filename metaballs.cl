kernel void metaballs(global const int2 *sources, int num, float charge, float limit, int apply, write_only image2d_t output){

    int x=get_global_id(0);
    int y=get_global_id(1);
    int2 pos;
    pos.x=x;
    pos.y=y;

    float sum=0.0f;

    int i=0;
    for(i=0;i<num;i++){
        float dx=(x-sources[i].x)/charge;
        float dy=(y-sources[i].y)/charge;
        float val=exp(-dx*dx-dy*dy);
        sum+=val;
    }

    if(apply)   sum=sum>limit?1.0f:0.0f;

    float4 color;
    color.x=sum;
    color.y=sum;
    color.z=sum;
    color.w=1.0f;

    write_imagef(output,pos,color);
}

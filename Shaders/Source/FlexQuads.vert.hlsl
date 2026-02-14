cbuffer Uniforms : register(b0, space1) {
    float2 window : packoffset(c0);
    float2 A : packoffset(c0.z);
    float2 B : packoffset(c1);
    float  C : packoffset(c1.z);
    int mode : packoffset(c1.w);
};

struct Output{
    float4 pos : SV_POSITION;
};

Output main( uint ID : SV_VertexID ){

    float2 P;

    switch( mode ){

        case 0: // crosshair
            switch(ID) {
                // top
                case 0:
                P = float2(A.x, A.y - B.x); break;
                // bottom
                case 1:
                P = float2(A.x, A.y + B.x); break;
                // left
                case 2:
                P = float2(A.x - B.x, A.y); break;
                // right
                case 3:
                P = float2(A.x + B.x, A.y); break;
            }
            break;

        case 1: // plain rect
            switch(ID) {
                // top-left
                case 0: 
                case 7: P = A; break;                            
                // top-right
                case 1:
                case 2: P = float2(A.x + B.x, A.y); break;       
                // bottom-right
                case 3:
                case 4: P = float2(A.x + B.x, A.y + B.y); break; 
                // bottom-left
                case 5:
                case 6: P = float2(A.x, A.y + B.y); break;       
            }
            break;

        case 2: // rect corners
            float2 a, b;
            a.x = A.x + 1;
            a.y = A.y + 1;
            b.x = B.x - 2;
            b.y = B.y - 2;
            switch(ID) {
                // Top-left
                case 0: 
                case 2: P = float2(a.x,             a.y);            break;
                case 1: P = float2(a.x + C,         a.y);            break;
                case 3: P = float2(a.x,             a.y + C);        break;
                // Top-right
                case 4:
                case 6: P = float2( a.x + b.x,      a.y );           break;
                case 5: P = float2( a.x + b.x - C,  a.y );           break;
                case 7: P = float2( a.x + b.x,      a.y + C );       break;
                // Bottom-left
                case  8:
                case 10: P = float2( a.x,           a.y + b.y );     break;
                case  9: P = float2( a.x + C,       a.y + b.y );     break;
                case 11: P = float2( a.x,           a.y + b.y - C ); break;
                // Bottom-right
                case 12:
                case 14: P = float2( a.x + b.x,     a.y + b.y );     break;
                case 13: P = float2( a.x + b.x - C, a.y + b.y );     break;
                case 15: P = float2( a.x + b.x,     a.y + b.y - C ); break;
            }
            break;

        case 3: // diamond
            switch(ID) {
                // top
                case 0:
                case 7: P = float2(A.x, A.y - B.x); break;
                // right
                case 1:
                case 2: P = float2(A.x + B.x, A.y); break;
                // bottom
                case 3:
                case 4: P = float2(A.x, A.y + B.x); break;
                // left
                case 5:
                case 6: P = float2(A.x - B.x, A.y); break;
            }
            break;
    }

    float2 ndc;
    ndc.x =  2 * (P.x / window.x) -1;
    ndc.y = -2 * (P.y / window.y) +1;

    Output OUT;
    OUT.pos = float4(ndc, 0.0, 1.0);
    return OUT;
}

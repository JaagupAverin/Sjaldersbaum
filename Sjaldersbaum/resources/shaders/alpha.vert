uniform float alpha;
void main()
{
    vec4 vertex    = gl_ModelViewMatrix * gl_Vertex;
    gl_Position    = gl_ProjectionMatrix * vertex;
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    gl_FrontColor  = vec4(gl_Color.r, gl_Color.g, gl_Color.b, gl_Color.a * alpha);
}
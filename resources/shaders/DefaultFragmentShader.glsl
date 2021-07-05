#version 410 core
#define MAX_LIGHTS 4
#define SHADOW_ERROR_MARGIN 0.01f
#define SUN 0
#define POINT 1
#define PLANE 2
#define SPOT 3

layout(location=0) out vec4 color;

in vec3 positionthru;
in vec3 normalthru;
in vec2 uvthru;
in vec4 shadowpositionthru[MAX_LIGHTS];
flat in uint numlightsthru;

//uniform sampler2DShadow shadowmaps[MAX_LIGHTS*2];
uniform sampler2DArrayShadow shadowmaps;
uniform vec3 lightpositions[MAX_LIGHTS];
uniform uint lighttypes[MAX_LIGHTS];
uniform uint lightresolutions[MAX_LIGHTS];

void main() {
    vec2 useuv=uvthru;
    vec3 lightposition;
    vec4 ambientlight=vec4(0.1f, 0.1f, 0.1f, 1.0f), diffuselight, tint;
    color=vec4(0, 1, 0, 1);
//    for(int x=0;x<numlightsthru;x++){
//        tint=vec4(1, 1, 1, 1);
//        lightposition=lightpositions[x];
//        if (lighttypes[x]==SUN) diffuselight=max(dot(normalthru, normalize(lightposition-positionthru)), 0)*vec4(1, 1, 1, 1);
////        if(texture())
//        if (texture(shadowmaps, vec4(shadowpositionthru[x].xyz, x))<shadowpositionthru[x].z-SHADOW_ERROR_MARGIN) tint+=vec4(0, 0, 1, 1);
////        if (sampleStaticDepthTextureAt(x)<shadowpositionthru[x].z-SHADOW_ERROR_MARGIN) tint+=vec4(0, 0, 1, 1);
////        if (texture(shadowmaps[1], shadowpositionthru[x].xyz)<shadowpositionthru[x].z-SHADOW_ERROR_MARGIN) tint+=vec4(1, 0, 0, 1);
////        if (texture(shadowmaps[2], shadowpositionthru[x+1].xyz)<shadowpositionthru[x+1].z-SHADOW_ERROR_MARGIN) tint+=vec4(0, 0, 1, 1);
//        //        if (sampleStaticDepthTextureAt(x)<shadowpositionthru[x].z-SHADOW_ERROR_MARGIN) tint+=vec4(0, 0, 1, 1);
////        if (texture(shadowmaps[3], shadowpositionthru[x+1].xyz)<shadowpositionthru[x+1].z-SHADOW_ERROR_MARGIN) tint+=vec4(1, 0, 0, 1);
//        color+=ambientlight+diffuselight;
//        color*=normalize(tint);
//    }
    //hey so if we flip the logic in here, we can just not compute a lot of lighting stuff whenever the fragment is in shadow
    for(int x=0;x<numlightsthru;x++){
        lightposition=lightpositions[x];
        ambientlight=vec4(0.1, 0.1, 0.1, 1);
        if(lighttypes[x]==SUN) diffuselight=max(dot(normalthru, normalize(lightposition-positionthru)), 0)*vec4(1, 1, 1, 1);
        else diffuselight=max(dot(normalthru, normalize(lightposition-positionthru)), 0)*vec4(100, 100, 100, 1)*1.0f/(pow(distance(positionthru, lightposition), 2));
        tint=vec4(1, 1, 1, 1);
        if(lighttypes[x]==SUN||lighttypes[x]==PLANE){
            //i really wish we could halve these calls by combining the textures on the c++ side, but we'll see if that works
            //after i actually get this working okay
//            if(texture(shadowmaps[2*x], shadowpositionthru[x].xyz)<shadowpositionthru[x].z-SHADOW_ERROR_MARGIN) tint+=vec4(0, 0, 1, 1);
            if(texture(shadowmaps, vec4(shadowpositionthru[x].xyz, x))<shadowpositionthru[x].z-SHADOW_ERROR_MARGIN) tint+=vec4(0, 0, 1, 1);
//            if(texture(shadowmaps[2*x+1], shadowpositionthru[x].xyz)<shadowpositionthru[x].z-SHADOW_ERROR_MARGIN) tint+=vec4(1, 0, 0, 1);
        }
        else if(lighttypes[x]==SPOT){
            //textureProj seems to be pretty costly...
//            if(textureProj(shadowmaps[2*x], shadowpositionthru[x])<(shadowpositionthru[x].z-SHADOW_ERROR_MARGIN)/shadowpositionthru[x].w) tint=vec4(0, 0, 1, 1);
//            if(textureProj(shadowmaps[2*x+1], shadowpositionthru[x])<(shadowpositionthru[x].z-SHADOW_ERROR_MARGIN)/shadowpositionthru[x].w) tint=vec4(0, 0, 1, 1);
        }
        color+=ambientlight+diffuselight;
        color*=normalize(tint);
    }
}
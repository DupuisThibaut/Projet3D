#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D tex;
uniform sampler2D textures[16];

uniform int nb;
uniform vec4 info[16];
uniform int id[16];

void main()
{             
    vec4 texCol = texture(tex, TexCoords);  

    for(int i=0;i<nb;i++){
        vec2 pos=info[i].xy;
        vec2 taille=info[i].zw;
        if(TexCoords.x>=pos.x && TexCoords.x<=pos.x+taille.x && TexCoords.y>=pos.y && TexCoords.y<=pos.y+taille.y){
            vec2 uv=(TexCoords-pos)/taille;
            uv.y=1.0-uv.y;
            vec4 col=texture(textures[i],uv);
            texCol=mix(texCol,col,col.a);
            // texCol=col;
        }
    }    


    // FragColor = vec4(texCol, 1.0);
    FragColor=texCol;
}

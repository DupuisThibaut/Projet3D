#version 430 core
layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform image2D imgOutput;

uniform vec3 uCamPos;
uniform mat4 uInvViewProj;
uniform ivec2 uResolution;
uniform int nbSphere;
uniform int nbSquare;
uniform int nbLight;
uniform int nbMesh;

// struct intersect{
// 	float t;
// 	vec3 normal;
// 	vec3
// }

struct Sphere{
	vec3 centre; 
	float rayon; 
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};
layout(std430,binding=1)buffer Spheres{Sphere spheres[];};

float intersectSphere(vec3 ro,vec3 rd,vec3 center,float radius){
    vec3 oc=ro-center;
	float a=dot(rd,rd);
    float b=2.0*dot(oc,rd);
    float c=dot(oc,oc)-(radius*radius);
    float disc=(b*b)-(4.0*a*c);
    if(disc<0.0)return -1.0;
    float t=(-b-sqrt(disc))/(2.0*a);
    if(t>0.0)return t;
    t=(-b+sqrt(disc))/(2.0*a);
    return(t>0.0)?t:-1.0;
}

struct Square{
	vec4 m_bottom_left;
	vec4 m_right_vector;
	vec4 m_up_vector;
	vec4 m_normal;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};
layout(std430,binding=2)buffer Squares{Square squares[];};

float intersectSquare(vec3 ro, vec3 rd, vec3 m_bottom_left, vec3 m_right_vector, vec3 m_up_vector, vec3 m_normal){
	if(dot(rd,m_normal)>=0.0)return -1.0;
	float d=dot(m_bottom_left,m_normal);
	float t=(d-dot(ro,m_normal))/dot(rd,m_normal);
	if(t<0.0)return -1.0;
	vec3 p=ro+t*rd;
	vec3 q=p-m_bottom_left;
	float proj1=dot(q,m_right_vector)/length(m_right_vector);
	float proj2=dot(q,m_up_vector)/length(m_up_vector);
	if((proj1<=length(m_right_vector) && proj1>=0.0) && (proj2<=length(m_up_vector) && proj2>=0.0)){
		return t;
	}
	return -1.0;
}

struct Light{
	vec3 pos;
	float rayon;
};
layout(std430,binding=3)buffer Lights{Light lights[];};


struct Vertex{
	vec4 position;
	vec4 normal;
};
layout(std430,binding=4)buffer Vertices{Vertex vertices[];};

struct Triangle{
	ivec4 indices;
};
layout(std430,binding=5)buffer Triangles{Triangle triangles[];};

struct Mesh{
	// int premierVertex;
	// int nbVertex;
	// int premierTriangle;
	// int nbTriangle;
	ivec4 info;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};
layout(std430,binding=6)buffer Meshes{Mesh meshes[];};

struct BVH{
	vec3 minp;
	int nb;
	vec3 maxp;
	int prof;
	ivec4 info;//0 child, 1 start, 2 count 
};
layout(std430,binding=7)buffer BVHS{BVH bvhs[];};

layout(std430, binding = 8) buffer DebugBuffer {
    float debugValues[];
};

vec3 normalTriangleTest;
vec3 normalTriangleFinal;

float intersectTriangle(vec3 ro, vec3 rd, vec3 v0, vec3 v1, vec3 v2){
	vec3 notNorm=cross(v1-v0,v2-v0);
	float norm=length(notNorm);
	vec3 m_normal=notNorm/norm;
	float area=norm/2.0;
	if(dot(m_normal,rd)==0.0)return -1.0;
	float d=dot(rd,m_normal);
	float t=(dot(v0-ro,m_normal))/d;
	vec3 p=ro+rd*t;
	if(t<=0.0)return -1.0;
	vec3 p0=v1-v0;
	vec3 p1=v2-v0;
	vec3 p2=p-v0;
	float d00=dot(p0,p0);
	float d01=dot(p0,p1);
	float d11=dot(p1,p1);
	float d20=dot(p2,p0);
	float d21=dot(p2,p1);
	float denom=d00*d11-d01*d01;
	float u1=(d11*d20-d01*d21)/denom;
	float u2=(d00*d21-d01*d20)/denom;
	float u0=1.0-u1-u2;
	if(u0<0.0||u1<0.0||u2<0.0||u0>1.0||u1>1.0||u2>1.0)return -1.0;
	return t;
}

float intersectBVH(vec3 ro, vec3 rd, vec3 minp, vec3 maxp) {
    vec3 inv=1.0/rd;
    vec3 tmin=(minp-ro)*inv;
    vec3 tmax=(maxp-ro)*inv;
    vec3 t1=min(tmin,tmax);
    vec3 t2=max(tmin,tmax);
    float tnear=max(max(t1.x,t1.y),t1.z);
    float tfar=min(min(t2.x,t2.y),t2.z);
    if(tfar<0.0||tnear>tfar){
        return -1.0;
    }
    return tnear;
}

float intersectMesh(vec3 ro, vec3 rd, int premierVertex, int premierTriangle, int nbTriangle, int premierBVH){
	int sp=0;
	int stack[64];
	stack[sp++]=premierBVH;
	float tmin=1e20;
	while(sp>0){
		int node=stack[--sp];
		float tbvh=intersectBVH(ro,rd,bvhs[node].minp.xyz,bvhs[node].maxp.xyz);
		if(tbvh>=0.0){
			int left=bvhs[node].info[0];
			int right=bvhs[node].info[1];
			if(left!=-1){
				stack[sp++]=left;
			}
			if(right!=1){
				stack[sp++]=right;
			}
			if(left==-1 && right==-1){
				int start=bvhs[node].info[2];
				int count=bvhs[node].info[3];
				for(int i=0;i<count;i++){
					int numeroTriangle=premierTriangle+start+i;
					int i0=triangles[numeroTriangle].indices[0]+premierVertex;
					int i1=triangles[numeroTriangle].indices[1]+premierVertex;
					int i2=triangles[numeroTriangle].indices[2]+premierVertex;
					vec3 v0=vertices[i0].position.xyz;
					vec3 v1=vertices[i1].position.xyz;
					vec3 v2=vertices[i2].position.xyz;
					float t=intersectTriangle(ro,rd,v0,v1,v2);
					if(t>0.0 && t<tmin){
						tmin=t;
						normalTriangleFinal=normalize(cross(v1-v0,v2-v0));
					}
				}
			}
		}
	}
	if(tmin==1e20)return -1.0;
	return tmin;
}



struct intersection{
	int hitIndex;
	float tmin;
	int inter;
	vec3 normal;
};

intersection intersectScene(vec3 ro, vec3 rd){
	intersection res;
	res.tmin=1e19;
	res.hitIndex=-1;
	res.inter=-1;
	
    for (int i=0;i<nbSphere;++i) {
        float t=intersectSphere(ro,rd,spheres[i].centre,spheres[i].rayon);
        if(t>0.0 && t<res.tmin){res.tmin=t;res.hitIndex=i;res.inter=1;}
    }

    for (int i=0;i<nbSquare;++i) {
        float t=intersectSquare(ro,rd,squares[i].m_bottom_left.xyz,squares[i].m_right_vector.xyz,squares[i].m_up_vector.xyz,squares[i].m_normal.xyz);
        if(t>0.0 && t<res.tmin){res.tmin=t;res.hitIndex=i;res.inter=2;}
    }

	if(intersectBVH(ro,rd,bvhs[0].minp.xyz,bvhs[0].maxp.xyz)>0.0){
		for(int i=0;i<nbMesh;i++){
			int b=meshes[i].info[1];
			float t=intersectMesh(ro,rd,meshes[i].info[0],meshes[i].info[2],meshes[i].info[3],b);
			if(t>0.0 && t<res.tmin){res.tmin=t;res.hitIndex=i;res.inter=3;}
		}
	}

	return res;
}


//Creer un vec3 random
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 randomInUnitSphere(vec2 seed, float radius) {
    float theta = rand(seed) * 2.0 * 3.14159265359;
    float phi = rand(seed * 2.0) * 3.14159265359;
    float r = rand(seed * 3.0) * radius;
    
    return vec3(
        r * sin(phi) * cos(theta),
        r * sin(phi) * sin(theta),
        r * cos(phi)
    );
}

vec3 couleur(vec3 ro, vec3 rd, intersection inter,ivec2 pix){
	int hitIndex=inter.hitIndex;
	float tmin=inter.tmin;
	int interObjet=inter.inter;
	vec3 l=vec3(0.8,0.8,0.8);
	vec3 finalColor=vec3(1.0,0.0,0.0);
    if(interObjet==1){
		vec3 light=lights[0].pos;
        vec3 p=ro+rd*tmin;
		vec3 L=light-p;
		float Ldist=length(L);
        vec3 n=(p-spheres[hitIndex].centre)/spheres[hitIndex].rayon;
		vec3 v=ro-p;
		L=normalize(L);
		v=normalize(v);
		n=normalize(n);
		float cosT=dot(n,L);
		vec3 r=reflect(-L,n);
		r=normalize(r);
		float cosA=dot(r,v);
		float shininess=spheres[hitIndex].specular.w;
		vec3 ambient=spheres[hitIndex].ambient.rgb;
		vec3 diffuse=spheres[hitIndex].diffuse.rgb;
		vec3 specular=spheres[hitIndex].specular.rgb;
		finalColor[0]=l[0]*ambient[0]+l[0]*diffuse[0]*cosT+l[0]*specular[0]*pow(cosA,shininess);
		finalColor[1]=l[1]*ambient[1]+l[1]*diffuse[1]*cosT+l[1]*specular[1]*pow(cosA,shininess);
		finalColor[2]=l[2]*ambient[2]+l[2]*diffuse[2]*cosT+l[2]*specular[2]*pow(cosA,shininess);
		vec3 ro2=p+n*0.00001;
		intersection intersectionLumiere=intersectScene(ro2,L);
		if(intersectionLumiere.inter>0 && intersectionLumiere.tmin<length(L))finalColor=vec3(0.0,0.0,0.0);
		// vec3 ro2=p+n*0.001;
		// intersection intersectionLumiere=intersectScene(ro2,L);
		// if(intersectionLumiere.inter>0 && intersectionLumiere.tmin<Ldist){
		// 	finalColor=vec3(0.0,0.0,0.0);
		// }
		// float nombreRayonOmbreDouce=0.0;
		// int nombreRayon=10;
		// float pourcentageOmbre=0.0;
		// vec3 nrd=L;
		// vec3 ro2=p+n*0.001;
		// for(int i=0;i<nombreRayon;i++){
		// 	vec3 rand=randomInUnitSphere(vec2(float(i),pix.x+pix.y),lights[0].rayon);			
		// 	vec3 nl=lights[0].pos+rand;
		// 	vec3 rd2=normalize(nl-p);
		// 	intersection intersectionLumiere=intersectScene(ro2,rd2);
		// 	if(intersectionLumiere.inter>0 && intersectionLumiere.tmin<length(rd2))nombreRayonOmbreDouce++;
		// }
		// pourcentageOmbre=nombreRayonOmbreDouce/nombreRayon;
		// finalColor*=(1.0-pourcentageOmbre);
    }else if(interObjet==2){
		vec3 light=lights[0].pos;
        vec3 p=ro+rd*tmin;
		vec3 L=light-p;
		float Ldist=length(L);
        vec3 n=squares[hitIndex].m_normal.xyz;
		vec3 v=ro-p;
		L=normalize(L);
		v=normalize(v);
		n=normalize(n);
		float cosT=dot(n,L);
		vec3 r=reflect(-L,n);
		r=normalize(r);
		float cosA=dot(r,v);
		float shininess=squares[hitIndex].specular.w;
		vec3 ambient=squares[hitIndex].ambient.rgb;
		vec3 diffuse=squares[hitIndex].diffuse.rgb;
		vec3 specular=squares[hitIndex].specular.rgb;
		finalColor[0]=l[0]*ambient[0]+l[0]*diffuse[0]*cosT+l[0]*specular[0]*pow(cosA,shininess);
		finalColor[1]=l[1]*ambient[1]+l[1]*diffuse[1]*cosT+l[1]*specular[1]*pow(cosA,shininess);
		finalColor[2]=l[2]*ambient[2]+l[2]*diffuse[2]*cosT+l[2]*specular[2]*pow(cosA,shininess);
		vec3 ro2=p+n*0.00001;
		intersection intersectionLumiere=intersectScene(ro2,L);
		if(intersectionLumiere.inter>0 && intersectionLumiere.tmin<length(L))finalColor=vec3(0.0,0.0,0.0);
		// vec3 ro2=p+n*0.001;
		// intersection intersectionLumiere=intersectScene(ro2,L);
		// if(intersectionLumiere.inter>0 && intersectionLumiere.tmin<Ldist){
		// 	finalColor=vec3(0.0,0.0,0.0);
		// }		
		// float nombreRayonOmbreDouce=0.0;
		// int nombreRayon=10;
		// float pourcentageOmbre=0.0;
		// vec3 newRay;
		// vec3 ro2=p+n*0.001;
		// for(int i=0;i<nombreRayon;i++){
		// 	newRay=light+randomInUnitSphere(vec2(float(i),pix.x+pix.y),lights[0].rayon);			
		// 	vec3 rd2=normalize(newRay-p);
		// 	intersection intersectionLumiere=intersectScene(ro2,rd2);
		// 	if(intersectionLumiere.inter>0 && intersectionLumiere.tmin<length(rd2))nombreRayonOmbreDouce++;
		// }
		// pourcentageOmbre=nombreRayonOmbreDouce/nombreRayon;
		// finalColor*=(1.0-pourcentageOmbre);
    }else if(interObjet==3){
		vec3 light=lights[0].pos;
        vec3 p=ro+rd*tmin;
		vec3 L=light-p;
        vec3 n=normalTriangleFinal;
		vec3 v=ro-p;
		L=normalize(L);
		v=normalize(v);
		n=normalize(n);
		float cosT=dot(n,L);
		vec3 r=reflect(-L,n);
		r=normalize(r);
		float cosA=dot(r,v);
		float shininess=meshes[hitIndex].specular.w;
		vec3 ambient=meshes[hitIndex].ambient.rgb;
		vec3 diffuse=meshes[hitIndex].diffuse.rgb;
		vec3 specular=meshes[hitIndex].specular.rgb;
		finalColor[0]=l[0]*ambient[0]+l[0]*diffuse[0]*cosT+l[0]*specular[0]*pow(cosA,shininess);
		finalColor[1]=l[1]*ambient[1]+l[1]*diffuse[1]*cosT+l[1]*specular[1]*pow(cosA,shininess);
		finalColor[2]=l[2]*ambient[2]+l[2]*diffuse[2]*cosT+l[2]*specular[2]*pow(cosA,shininess);
		vec3 ro2=p+n*0.00001;
		intersection intersectionLumiere=intersectScene(ro2,L);
		if(intersectionLumiere.inter>0 && intersectionLumiere.tmin<length(L))finalColor=vec3(0.0,0.0,0.0);
		// float nombreRayonOmbreDouce=0.0;
		// int nombreRayon=10;
		// float pourcentageOmbre=0.0;
		// vec3 newRay;
		// vec3 ro2=p+n*0.001;
		// for(int i=0;i<nombreRayon;i++){
		// 	newRay=light+randomInUnitSphere(vec2(float(i),pix.x+pix.y),lights[0].rayon);			
		// 	vec3 rd2=normalize(newRay-p);
		// 	intersection intersectionLumiere=intersectScene(ro2,rd2);
		// 	if(intersectionLumiere.inter>0 && intersectionLumiere.tmin<length(rd2))nombreRayonOmbreDouce++;
		// }
		// pourcentageOmbre=nombreRayonOmbreDouce/nombreRayon;
		// finalColor*=(1.0-pourcentageOmbre);
    }else{
        finalColor=vec3(0.68,0.85,0.90);
    }
	return finalColor;
}

void main(){
	//Création du rayon
    ivec2 pix=ivec2(gl_GlobalInvocationID.xy);
	// debugValues[pix.x*uResolution.x+pix.y] = 12.5;
    if(pix.x>=uResolution.x||pix.y>=uResolution.y)return;
    vec2 ndc=(vec2(pix)+0.5)/vec2(uResolution)*2.0-1.0;
    vec4 clip=vec4(ndc,-1.0,1.0);
    vec4 world=uInvViewProj*clip;
    world/=world.w;
    vec3 ro=uCamPos;
    vec3 rd=normalize(world.xyz-ro);
    
	intersection inter=intersectScene(ro,rd);
    vec3 finalColor=couleur(ro,rd,inter,pix);

    imageStore(imgOutput,pix,vec4(finalColor,1.0));
}
#version 430 core
// #extension GL_ARB_bindless_texture : require
layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform image2D imgOutput;
layout(rgba32f, binding = 1) uniform image2D imgAccum;

uniform vec3 uCamPos;
uniform mat4 uInvViewProj;
uniform ivec2 uResolution;
uniform int frameCount;
uniform int nbSphere;
uniform int nbSquare;
uniform int nbLight;
uniform int nbMesh;
uniform int nbParticule;
uniform int resetAccum;
uniform uint accum;
uniform int blinn;

#define PI 3.1415926538

vec2 uvTest;
vec2 uvFinal;

// struct intersect{
// 	float t;
// 	vec3 normal;
// 	vec3
// }

struct Particule{
	vec4 position;
	uvec2 text;
	uvec2 padding;
};
layout(std430,binding=11)buffer Particules{Particule particules[];};

struct Ray{
	vec3 origin;
	vec3 direction;
};

struct World{
	mat4 modelMat;
	mat4 invModelMatrix;
	mat3 normalMat;
	vec4 testSphere;
};
layout(std430,binding=8)buffer Worlds{World worlds[];};

struct Sphere{
	vec3 centre; 
	float rayon; 
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	uvec2 text;
	uvec2 padding;
};
layout(std430,binding=9)buffer Spheres{Sphere spheres[];};

float secondeIntersection;

float intersectSphere(Ray rayon,vec3 center,float radius){
	vec3 ro=rayon.origin;
	vec3 rd=rayon.direction;
    vec3 oc=ro-center;
	float a=dot(rd,rd);
    float b=2.0*dot(oc,rd);
    float c=dot(oc,oc)-(radius*radius);
    float disc=(b*b)-(4.0*a*c);
    if(disc<0.0)return -1.0;
    float t1=(-b-sqrt(disc))/(2.0*a);
    float t2=(-b+sqrt(disc))/(2.0*a);
	vec3 n=normalize((ro+rd*t1)-center);
	float theta=atan(n.z,n.x);
	float phi=asin(n.y);
	uvTest=vec2(0.5+(theta+PI)/(2.0*PI),0.5-phi/PI);
    if(t1>0.0 && t1<t2){
		secondeIntersection=t2;
		return t1;
	}
    if(t2>0.0 && t1>t2){
		secondeIntersection=t1;
		return t2;
	}
    return -1.0;
}

struct Square{
	vec4 m_bottom_left;
	vec4 m_right_vector;
	vec4 m_up_vector;
	vec4 m_normal;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	uvec2 text;
	uvec2 padding;
};
layout(std430,binding=10)buffer Squares{Square squares[];};

float intersectSquare(Ray rayon, vec3 m_bottom_left, vec3 m_right_vector, vec3 m_up_vector, vec3 m_normal, float lengthUV, float lengthRV){
	vec3 ro=rayon.origin;
	vec3 rd=rayon.direction;
	if(dot(rd,m_normal)>=0.0)return -1.0;
	// if(dot(rd,m_normal)>=0.0)m_normal=-m_normal;
	float d=dot(m_bottom_left,m_normal);
	float t=(d-dot(ro,m_normal))/dot(rd,m_normal);
	if(t<0.0)return -1.0;
	vec3 p=ro+t*rd;
	vec3 q=p-m_bottom_left;
	float proj1=dot(q,m_right_vector)/lengthRV;
	float proj2=dot(q,m_up_vector)/lengthUV;
	// float proj1=dot(q,m_right_vector)/length(m_right_vector);
	// float proj2=dot(q,m_up_vector)/length(m_up_vector);
	if((proj1<=1.0 && proj1>=0.0) && (proj2<=1.0 && proj2>=0.0)){
		uvTest=vec2(proj1,proj2);
		return t;
	}
	return -1.0;
}

struct Light{
	vec3 pos;
	float rayon;
	vec3 color;
	float padding;
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
	uvec2 text;
	uvec2 padding;
};
layout(std430,binding=6)buffer Meshes{Mesh meshes[];};

struct BVH{
	vec4 minp;
	vec4 maxp;
	ivec4 info;//0 child, 1 start, 2 count 
};
layout(std430,binding=7)buffer BVHS{BVH bvhs[];};

vec3 normalTriangleTest;
vec3 normalTriangleFinal;

vec3 wTriangle;

float intersectTriangle(Ray rayon, vec3 v0, vec3 v1, vec3 v2){
	vec3 ro=rayon.origin;
	vec3 rd=rayon.direction;
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
	wTriangle=vec3(u0,u1,u2);
	return t;
}

float intersectBVH(Ray rayon, vec3 minp, vec3 maxp){
	vec3 ro=rayon.origin;
	vec3 rd=rayon.direction;
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

float intersectMesh(Ray rayon, int indice){
	vec3 ro=rayon.origin;
	vec3 rd=rayon.direction;
    int premierVertex=meshes[indice].info[0];
    int premierBVH=meshes[indice].info[1];
    int premierTriangle=meshes[indice].info[2];
    int stack[64];
    int sp=0;
    stack[sp++]=premierBVH;
    float tmin=1e20;
    while(sp>0){
        int node=stack[--sp];
        float tnode=intersectBVH(rayon,bvhs[node].minp.xyz,bvhs[node].maxp.xyz);
        if(tnode<0.0||tnode>tmin)continue;
        int left=bvhs[node].info[0];
        int right=bvhs[node].info[1];
        if(left==-1 && right==-1) {
            int start=bvhs[node].info[2];
            int count=bvhs[node].info[3];
            for(int i=0;i<count;i++){
                int tri=premierTriangle+start+i;
                int i0=triangles[tri].indices[0]+premierVertex;
                int i1=triangles[tri].indices[1]+premierVertex;
                int i2=triangles[tri].indices[2]+premierVertex;
                vec3 v0=vertices[i0].position.xyz;
                vec3 v1=vertices[i1].position.xyz;
                vec3 v2=vertices[i2].position.xyz;
                float t=intersectTriangle(rayon,v0,v1,v2);
                if(t>0 && t<tmin){
                    tmin=t;
                    normalTriangleFinal=normalize(cross(v1-v0,v2-v0));
					uvTest=vec2(wTriangle.x*vertices[i0].position.w+wTriangle.y*vertices[i1].position.w+wTriangle.z*vertices[i2].position.w,wTriangle.x*vertices[i0].normal.w+wTriangle.y*vertices[i1].normal.w+wTriangle.z*vertices[i2].normal.w);
                }
            }
			if(tmin!=1e20)return tmin;
            continue;
        }
        float tLeft=(left!=-1)? intersectBVH(rayon,bvhs[left].minp.xyz,bvhs[left].maxp.xyz):-1;
        float tRight=(right!=-1)? intersectBVH(rayon,bvhs[right].minp.xyz,bvhs[right].maxp.xyz):-1;
        if(tLeft>0 && tRight>0){
            if(tLeft<tRight){
                stack[sp++]=right;
                stack[sp++]=left;
            }else{
                stack[sp++]=left;
                stack[sp++]=right;
            }
        }else if(tLeft>0){
            stack[sp++]=left;
        }else if(tRight>0){
            stack[sp++]=right;
        }
    }
    return(tmin==1e20)? -1.0 : tmin;
}

struct intersection{
	int hitIndex;
	float tmin;
	int inter;
	vec3 normal;
};

float finalSecondeIntersectionSphere;

intersection intersectScene(Ray rayon){
	vec3 ro=rayon.origin;
	vec3 rd=rayon.direction;
	intersection res;
	res.tmin=1e19;
	res.hitIndex=-1;
	res.inter=-1;
	
    for (int i=0;i<nbSphere;++i) {
        float t=intersectSphere(rayon,spheres[i].centre,spheres[i].rayon);
        if(t>0.0 && t<res.tmin){res.tmin=t;res.hitIndex=i;res.inter=1;finalSecondeIntersectionSphere=secondeIntersection;uvFinal=uvTest;}
    }

    for (int i=0;i<nbSquare;++i) {
        float t=intersectSquare(rayon,squares[i].m_bottom_left.xyz,squares[i].m_right_vector.xyz,squares[i].m_up_vector.xyz,squares[i].m_normal.xyz,squares[i].m_up_vector[3],squares[i].m_right_vector[3]);
        if(t>0.0 && t<res.tmin){res.tmin=t;res.hitIndex=i;res.inter=2;uvFinal=uvTest;}
    }

	for(int i=0;i<nbParticule;i++){
		float t=intersectSphere(rayon,particules[i].position.xyz,particules[i].position.w);
		if(t>0.0 && t<res.tmin){res.tmin=t;res.hitIndex=i;res.inter=4;finalSecondeIntersectionSphere=secondeIntersection;uvFinal=uvTest;}
	}

	// if(intersectBVH(roLocal,rdLocal,bvhs[0].minp.xyz,bvhs[0].maxp.xyz)>0.0){
		for(int i=0;i<nbMesh;i++){
			mat4 model=worlds[i].modelMat;
			mat4 invModelMatrix=worlds[i].invModelMatrix;
			// vec3 centre=(model*vec4(worlds[i].testSphere.xyz,1.0)).xyz;
			// float testS=intersectSphere(ro,rd,worlds[i].testSphere.xyz,worlds[i].testSphere.w);
			// if(testS<0.0 || testS>res.tmin)continue;
			vec3 roLocal=(invModelMatrix*vec4(ro,1.0)).xyz;
			vec3 rdLocal=normalize((invModelMatrix*vec4(rd,0.0)).xyz);
			Ray rayonLocal;rayonLocal.origin=roLocal;rayonLocal.direction=rdLocal;
			float t=intersectMesh(rayonLocal,i);
			if(t>0.0){
				vec3 pLocal=roLocal+rdLocal*t;
				vec3 pMonde=(model*vec4(pLocal,1.0)).xyz;
				float t2=length(pMonde-ro);
				if(t2<res.tmin){
					mat3 normalMat=worlds[i].normalMat;
					// mat3 normalMat=transpose(inverse(mat3(model)));
					normalTriangleFinal=normalize(normalMat*normalTriangleFinal);
					// normalTriangleFinal=normalize(mat3(model)*normalTriangleFinal);
					res.tmin=t2;res.hitIndex=i;res.inter=3;uvFinal=uvTest;
				}
			}
		}
	// }

	return res;
}

// LES FONCTIONS RANDOMS ///////////////////////////////////////////////////////////////////////////////////////////////////////


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

float hash13(vec3 p3) {
    p3 = fract(p3 * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 randomInSphere(vec3 seed) {
    float u = hash13(seed);
    float v = hash13(seed + 12.345);
    float w = hash13(seed + 98.234);

    float theta = 2.0 * 3.14159265 * u;
    float phi   = acos(1.0 - 2.0 * v);
    float r     = pow(w, 1.0/3.0);

    float sinPhi = sin(phi);
    return r * vec3(cos(theta)*sinPhi,
                    sin(theta)*sinPhi,
                    cos(phi));
}

// hash 32-bit → float dans [0,1]
float hash1(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return float(x) * (1.0 / 4294967296.0); // 1/2^32
}

// hash 32-bit → vec3 dans [0,1]
vec3 hash3(uint x)
{
    return vec3(
        hash1(x),
        hash1(x * 747796405u + 2891336453u),
        hash1(x * 2777u + 1234567u)
    );
}

vec3 randomDirection(uint seed)
{
    vec3 r = hash3(seed);   // 3 random floats

    float z = r.x * 2.0 - 1.0;        // [-1,1]
    float a = r.y * 6.28318530718;   // angle phi 0..2π
    float w = sqrt(1.0 - z*z);

    return vec3(
        w * cos(a),
        w * sin(a),
        z
    );
}

// vec3 randomInUnitSphere(vec2 seed, float radius)
// {
//     float u = rand(seed);
//     float v = rand(seed * 2.0);
//     float w = rand(seed * 3.0);
//     float theta = 2.0 * 3.14159265359 * u;
//     float phi   = acos(1.0 - 2.0 * v);
//     float r = radius * pow(w, 1.0 / 3.0);
//     float sinPhi = sin(phi);
//     return vec3(
//         r * sinPhi * cos(theta),
//         r * sinPhi * sin(theta),
//         r * cos(phi)
//     );
// }

uint wang_hash(uint seed)
{
    seed = (seed ^ uint(61)) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

// Generate random float in [0,1)
float random_float(uint seed)
{
    seed = wang_hash(seed);
    return float(seed) * (1.0 / 4294967296.0);
}

float random01(uint seed){
	return float(wang_hash(seed) & uint(0xFFFFFF))/float(0xFFFFFF);
}

double randomDouble(vec2 seed) {
    float r1 = rand(seed);
    float r2 = rand(seed + 123.456);
    return double(r1) + double(r2) * 1e-6;
}

uint rng_state;

uint jenkins_hash(uint x)
{
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

void init_rng(uvec2 pixel, uint width, uint frame_count)
{
    uint seed = (pixel.x + pixel.y * width) ^ jenkins_hash(frame_count);
    rng_state = jenkins_hash(seed);
}

uint xorshift32()
{
    uint x = rng_state;
    x ^= (x << 13u);
    x ^= (x >> 17u);
    x ^= (x << 5u);
    rng_state = x;
    return x;
}

float rand_f32()
{
    // 0x3F800000 = float 1.0
    uint bits = 0x3F800000u | (xorshift32() >> 9u);
    float f = uintBitsToFloat(bits);
    return f - 1.0;
}

vec3 randomHemisphere(vec3 n, uint seed) {
    vec3 r = randomDirection(seed);
    return dot(r, n) < 0.0 ? -r : r;
}

vec3 random_hemisphere_direction(vec3 normal, uint seed)
{
	// Generate two random numbers
	float r1 = random01(seed++);
	float r2 = random01(seed++);
	
	// Cosine-weighted hemisphere sampling
	float phi = 2.0 * PI * r1;
	// float cos_theta = sqrt(1.0 - r2);
	// float sin_theta = sqrt(r2);
	float cos_theta = sqrt(r2);
	float sin_theta = sqrt(1.0 - r2);
	
	// Create local coordinate system around normal
	vec3 tangent;
	if (abs(normal.x) > 0.1) {
		tangent = normalize(cross(vec3(0.0, 1.0, 0.0), normal));
	} else {
		tangent = normalize(cross(vec3(1.0, 0.0, 0.0), normal));
	}
	vec3 bitangent = cross(normal, tangent);
	
	// Transform from local to world space
	vec3 direction = cos(phi) * sin_theta * tangent +
	                   sin(phi) * sin_theta * bitangent +
	                   cos_theta * normal;
	
	return normalize(direction);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool testOmbre(Ray rayon, float dist){
	vec3 ro=rayon.origin;
	vec3 rd=rayon.direction;
	
	for (int i=0;i<nbSphere;++i) {
		float t=intersectSphere(rayon,spheres[i].centre,spheres[i].rayon);
		if(t>0.0 && t<dist){
			if(spheres[i].padding[1]!=2)
			return true;}
	}
	for (int i=0;i<nbSquare;++i) {
		float t=intersectSquare(rayon,squares[i].m_bottom_left.xyz,squares[i].m_right_vector.xyz,squares[i].m_up_vector.xyz,squares[i].m_normal.xyz,squares[i].m_up_vector[3],squares[i].m_right_vector[3]);
		if(t>0.0 && t<dist){return true;}
	}
	for(int i=0;i<nbMesh;i++){
		mat4 model=worlds[i].modelMat;
		mat4 invModelMatrix=worlds[i].invModelMatrix;
		vec3 roLocal=(invModelMatrix*vec4(ro,1.0)).xyz;
		vec3 rdLocal=normalize((invModelMatrix*vec4(rd,0.0)).xyz);
		Ray rayonLocal;rayonLocal.origin=roLocal;rayonLocal.direction=rdLocal;
		float t=intersectMesh(rayonLocal,i);
		if(t>0.0){
			vec3 pLocal=roLocal+rdLocal*t;
			vec3 pMonde=(model*vec4(pLocal,1.0)).xyz;
			float t2=length(pMonde-ro);
			if(t2<dist){
				return true;
			}
		}
	}
	return false;
}

float ombre(vec3 p, vec3 n, vec2 pix, vec3 light){
	float nombreRayonOmbreDouce=0.0;
	int nombreRayon=1;
	float pourcentageOmbre=0.0;
	vec3 newRay;
	vec3 ro=p+n*0.01;
	for(int i=0;i<nombreRayon;i++){
		// newRay=light+randomInSphere(vec3(pix,i))*lights[0].rayon;
		uint seed=uint(pix.x)+uint(pix.y)*92837111u+uint(i)*1234567u;
		newRay=light+vec3(random_float(seed)-0.5,random_float(seed)-0.5,random_float(seed)-0.5)*lights[0].rayon;
		// newRay=light+randomDirection(seed)*lights[0].rayon;
		// vec3(random_float(seed)-0.5,random_float(seed)-0.5,random_float(seed)-0.5)
		vec3 L=newRay-p;
		float dist=length(L);			
		vec3 rd=normalize(L);
		Ray rayon;rayon.origin=ro;rayon.direction=rd;
		if(testOmbre(rayon,dist))nombreRayonOmbreDouce++;
	}
	pourcentageOmbre=nombreRayonOmbreDouce/float(nombreRayon);
	return 1.0-pourcentageOmbre;
}

vec3 l=vec3(0.8,0.8,0.8);

vec3 couleurSphere(Ray rayon,float tmin,int hitIndex,vec2 pix){
	vec3 ro=rayon.origin;
	vec3 rd=rayon.direction;
	vec3 finalColor=vec3(1.0,1.0,1.0);
	// if(spheres[hitIndex].padding[0]==1){
	// 	sampler2D tex=sampler2D(spheres[hitIndex].text);
	// 	finalColor=texture(tex,uvFinal).rgb;
	// }else{
	// 	finalColor=vec3(1.0,1.0,1.0);
	// }
	for(int i=0;i<nbLight;i++){
		l=lights[i].color;
		vec3 light=lights[i].pos;
		vec3 p=ro+rd*tmin;
		vec3 L=light-p;
		float Ldist=length(L);
		vec3 n=(p-spheres[hitIndex].centre)/spheres[hitIndex].rayon;
		vec3 v=ro-p;
		L=normalize(L);
		Ray rayonLumiere;rayonLumiere.origin=p;rayonLumiere.direction=L;
		if(testOmbre(rayonLumiere,Ldist)){
			return vec3(0.0);
		}
		v=normalize(v);
		float cosT=max(dot(n,L),0.0);
		vec3 r=reflect(-L,n);
		r=normalize(r);
		float cosA=max(dot(r,v),0.0);
		float shininess=spheres[hitIndex].specular.w;
		vec3 ambient=spheres[hitIndex].ambient.rgb;
		vec3 diffuse=spheres[hitIndex].diffuse.rgb;
		vec3 specular=spheres[hitIndex].specular.rgb;
		finalColor[0]*=l[0]*ambient[0]+l[0]*diffuse[0]*cosT+l[0]*specular[0]*pow(cosA,shininess);
		finalColor[1]*=l[1]*ambient[1]+l[1]*diffuse[1]*cosT+l[1]*specular[1]*pow(cosA,shininess);
		finalColor[2]*=l[2]*ambient[2]+l[2]*diffuse[2]*cosT+l[2]*specular[2]*pow(cosA,shininess);
		// finalColor*=ombre(p,n,pix,light);
	}
	return finalColor;
}

vec3 couleurSquare(Ray rayon,float tmin,int hitIndex,vec2 pix){
	vec3 ro=rayon.origin;
	vec3 rd=rayon.direction;
	vec3 finalColor=vec3(1.0,1.0,1.0);
	// if(squares[hitIndex].padding[0]==1){
	// 	sampler2D tex=sampler2D(squares[hitIndex].text);
	// 	finalColor=texture(tex,uvFinal).rgb;
	// }else{
	// 	finalColor=vec3(1.0,1.0,1.0);
	// }
	for(int i=0;i<nbLight;i++){
		l=lights[i].color;
		vec3 light=lights[i].pos;
		vec3 p=ro+rd*tmin;
		vec3 L=light-p;
		float Ldist=length(L);
		L=normalize(L);
		Ray rayonLumiere;rayonLumiere.origin=p+vec3(0.001);rayonLumiere.direction=L;
		if(testOmbre(rayonLumiere,Ldist)){
			return vec3(0.0);
		}
		vec3 n=squares[hitIndex].m_normal.xyz;
		vec3 v=ro-p;
		v=normalize(v);
		float cosT=max(dot(n,L),0.0);
		vec3 r=reflect(-L,n);
		r=normalize(r);
		float cosA=max(dot(r,v),0.0);
		float shininess=squares[hitIndex].specular.w;
		vec3 ambient=squares[hitIndex].ambient.rgb;
		vec3 diffuse=squares[hitIndex].diffuse.rgb;
		vec3 specular=squares[hitIndex].specular.rgb;
		finalColor[0]*=l[0]*ambient[0]+l[0]*diffuse[0]*cosT+l[0]*specular[0]*pow(cosA,shininess);
		finalColor[1]*=l[1]*ambient[1]+l[1]*diffuse[1]*cosT+l[1]*specular[1]*pow(cosA,shininess);
		finalColor[2]*=l[2]*ambient[2]+l[2]*diffuse[2]*cosT+l[2]*specular[2]*pow(cosA,shininess);
		// finalColor*=ombre(p,n,pix,light);
	}
	return finalColor;
}

vec3 couleurMesh(Ray rayon,float tmin,int hitIndex,vec2 pix){
	vec3 ro=rayon.origin;
	vec3 rd=rayon.direction;
	vec3 finalColor=vec3(1.0,1.0,1.0);
	// if(meshes[hitIndex].padding[0]==1){
	// 	sampler2D tex=sampler2D(meshes[hitIndex].text);
	// 	finalColor=texture(tex,uvFinal).rgb;
	// 	// return finalColor;
	// }else{
	// 	finalColor=vec3(1.0,1.0,1.0);
	// }
	for(int i=0;i<nbLight;i++){
		l=lights[i].color;
		vec3 light=lights[i].pos;
		vec3 p=ro+rd*tmin;
		vec3 L=light-p;
		float Ldist=length(L);
		L=normalize(L);
		Ray rayonLumiere;rayonLumiere.origin=p;rayonLumiere.direction=L;
		if(testOmbre(rayonLumiere,Ldist)){
			return vec3(0.0);
		}
		vec3 n=normalTriangleFinal;
		vec3 v=ro-p;
		v=normalize(v);
		float cosT=max(dot(n,L),0.0);
		vec3 r=reflect(-L,n);
		r=normalize(r);
		float cosA=max(dot(r,v),0.0);
		float shininess=meshes[hitIndex].specular.w;
		vec3 ambient=meshes[hitIndex].ambient.rgb;
		vec3 diffuse=meshes[hitIndex].diffuse.rgb;
		vec3 specular=meshes[hitIndex].specular.rgb;
		finalColor[0]*=l[0]*ambient[0]+l[0]*diffuse[0]*cosT+l[0]*specular[0]*pow(cosA,shininess);
		finalColor[1]*=l[1]*ambient[1]+l[1]*diffuse[1]*cosT+l[1]*specular[1]*pow(cosA,shininess);
		finalColor[2]*=l[2]*ambient[2]+l[2]*diffuse[2]*cosT+l[2]*specular[2]*pow(cosA,shininess);
		// finalColor*=ombre(p,n,pix,light);
	}
	return finalColor;
}

vec3 couleurParticule(int hitIndex){
	vec3 finalColor=vec3(1.0,1.0,1.0);
	// if(particules[hitIndex].padding[0]==1){
	// 	sampler2D tex=sampler2D(particules[hitIndex].text);
	// 	finalColor=texture(tex,uvFinal).rgb;
	// }else{
	// 	finalColor=vec3(0.0,0.6,1.0);
	// }
	finalColor=vec3(0.0,0.6,1.0);
	return finalColor;
}

Ray computeReflection(vec3 ro, vec3 n, vec3 p, ivec2 pix){
	vec3 v=ro-2*dot(ro,n)*n;
	v=normalize(v);
	vec3 vo=p+v*0.01;
	Ray result;
	result.origin=vo;
	result.direction=v;
	return result;
}

Ray computeRefraction(Ray rayon, vec3 n, vec3 p, ivec2 pix, float index, int type, int indice){
	vec3 ro=rayon.origin;
	vec3 rd=rayon.direction;
	vec3 finalColor=vec3(1.0,1.0,1.0);
	vec3 unit_dir=normalize(rd);
	n=normalize(n);
	float ri;
	if(dot(unit_dir,n)<0){
		ri=1.0/index;
	}else{
		ri=index;
		n=-n;
	}
	float cos_theta=min(dot(-unit_dir,n),1.0);
	float sin_theta=sqrt(1.0-cos_theta*cos_theta);
	bool cannot_refract=(ri*sin_theta)>1.0;
	float r0=(1-ri)/(1+ri);
	r0=r0*r0;
	float reflectance=r0+(1-r0)*pow((1-cos_theta),5);
	//  || reflectance>randomDouble(vec2(uint(pix.x),uint(pix.y)*92837111u))
	if(cannot_refract){
		return computeReflection(rd,n,p,pix);
	}else{
		vec3 r_out_perp=ri*(unit_dir+cos_theta*n);
		vec3 r_out_parallel=-sqrt(abs(1.0-dot(r_out_perp,r_out_perp)))*n;
		vec3 v=r_out_perp+r_out_parallel;
		v=normalize(v);
		vec3 vo=p+v*0.01;
		Ray result;
		result.origin=vo;
		result.direction=v;
		return result;
	}
}

// vec3 computeMetalic(vec3 rd, vec3 n, vec3 p, ivec2 pix, vec3 finalColor){
// 	float metallic=0.1;
// 	float reflectStrength=metallic;
// 	float diffuseStrength=1.0-metallic;
// 	vec3 reflectedColor=vec3(0.0,0.0,0.0);
// 	Ray rayon;
// 	if(reflectStrength>0.0) {
// 		rayon=computeReflection(rd,n,p,pix);
// 	}
// 	return diffuseStrength*finalColor+reflectStrength*reflectedColor;
// }

vec3 couleur(Ray rayon,ivec2 pix,uint seed){
	vec3 finalColor=vec3(1.0,1.0,1.0);
	int nbRayons=3;
	Ray testRayon=rayon;
	for(int i=0;i<nbRayons;i++){
		intersection inter=intersectScene(testRayon);
		vec3 ro=testRayon.origin;
		vec3 rd=testRayon.direction;
		int hitIndex=inter.hitIndex;
		float tmin=inter.tmin;
		int interObjet=inter.inter;
		vec3 testColor=vec3(1.0,1.0,1.0);
		if(interObjet==1){
			vec3 p=ro+rd*tmin;
			vec3 n=(p-spheres[hitIndex].centre)/spheres[hitIndex].rayon;
			testColor*=couleurSphere(testRayon,tmin,hitIndex,pix);
			if(spheres[hitIndex].padding[1]==1){
				testRayon=computeReflection(rd,n,p,pix);
			}
			else if(spheres[hitIndex].padding[1]==2){
				testRayon=computeRefraction(testRayon,n,p,pix,0.75,1,hitIndex);
				testRayon.origin=ro+rd*finalSecondeIntersectionSphere;
			}
			else if(spheres[hitIndex].padding[1]==3){
				// testRayon=computeMetalic(rd,n,p,pix,testColor);
				// float metallic=0.1;
				// float reflectStrength=metallic;
				// float diffuseStrength=1.0-metallic;
				// testRayon=computeReflection(rd,n,p,pix);
				// testColor*=diffuseStrength;
				// fn schlick_fresnel_vec3(f0: vec3f, cos_theta: f32) -> vec3f {
				// 	let u = 1 - cos_theta;
				// 	return mix(f0, vec3(1.), u * u * u * u * u);
				// }
			}else{
				testRayon.origin=p+n*0.001;
				testRayon.direction=random_hemisphere_direction(n,seed);
				if(bool(blinn)){
					finalColor*=testColor;
					break;
				}
			}
		}else if(interObjet==2){
			vec3 p=ro+rd*tmin;
			vec3 n=squares[hitIndex].m_normal.xyz;
			testColor*=couleurSquare(testRayon,tmin,hitIndex,pix);
			if(squares[hitIndex].padding[1]==1){
				testRayon=computeReflection(rd,n,p,pix);
			}
			else if(squares[hitIndex].padding[1]==2){
				testRayon=computeRefraction(testRayon,n,p,pix,0.75,2,hitIndex);
			}
			else if(squares[hitIndex].padding[1]==3){
				// testRayon=computeMetalic(rd,n,p,pix,testColor);
			}else{
				testRayon.origin=p+n*0.001;
				testRayon.direction=random_hemisphere_direction(n,seed);
				if(bool(blinn)){
					finalColor*=testColor;
					break;
				}
			}
		}else if(interObjet==3){
			vec3 p=ro+rd*tmin;
			vec3 n=normalTriangleFinal;
			testColor*=couleurMesh(testRayon,tmin,hitIndex,pix);
			if(meshes[hitIndex].padding[1]==1){
				testRayon=computeReflection(rd,n,p,pix);
			}
			if(meshes[hitIndex].padding[1]==2){
				testRayon=computeRefraction(testRayon,n,p,pix,0.75,3,hitIndex);
			}
			if(meshes[hitIndex].padding[1]==3){
				// testRayon=computeMetalic(rd,n,p,pix,testColor);
			}else{
				testRayon.origin=p+n*0.001;
				testRayon.direction=random_hemisphere_direction(n,seed);
				if(bool(blinn)){
					finalColor*=testColor;
					break;
				}
			}
		}else if(interObjet==4){
			vec3 p=ro+rd*tmin;
			vec3 n=(p-particules[hitIndex].position.xyz)/particules[hitIndex].position.w;
			testColor*=couleurParticule(hitIndex);
			if(particules[hitIndex].padding[1]==1){
				testRayon=computeReflection(rd,n,p,pix);
			}else if(particules[hitIndex].padding[1]==2){
				testRayon=computeRefraction(testRayon,n,p,pix,0.99,4,hitIndex);
				testRayon.origin=ro+rd*finalSecondeIntersectionSphere;
			}else if(particules[hitIndex].padding[1]==3){
				
			}
			// finalColor*=vec3(0.0,1.0,1.0);
			else{
				finalColor*=testColor;
				break;
			}
		}else{
			finalColor*=vec3(0.68,0.85,0.90);
			break;
		}
		// finalColor+=testColor;
		finalColor*=testColor;
	}
	// finalColor=testColor;
	return finalColor;
}

void main(){
	//Création du rayon
    ivec2 pix=ivec2(gl_GlobalInvocationID.xy);
    if(pix.x>=uResolution.x||pix.y>=uResolution.y)return;
	init_rng(uvec2(pix),uint(uResolution.x),uint(frameCount));
    vec2 ndc=(vec2(pix)+0.5)/vec2(uResolution)*2.0-1.0;
    vec4 clip=vec4(ndc,-1.0,1.0);
    vec4 world=uInvViewProj*clip;
    world/=world.w;
	Ray rayon;
    rayon.origin=uCamPos;
    rayon.direction=normalize(world.xyz-rayon.origin);

	uint seed=(pix.x * 1973u + pix.y * 9277u + frameCount * 26699u) | 1u;

    vec3 finalColor=couleur(rayon,pix,seed);

	//sans accumulation temporelle
	if(bool(blinn)){
    	imageStore(imgOutput,pix,vec4(finalColor,1.0));
	}else{
		if(bool(resetAccum) || frameCount == 0){
			imageStore(imgAccum, pix, vec4(finalColor, 1.0));
			imageStore(imgOutput, pix, vec4(finalColor, 1.0));
		} else {
			vec3 prev = imageLoad(imgAccum, pix).rgb;
			float t=float(accum)/float(accum+1);
			vec3 acc=prev*t+finalColor*(1.0-t);
			imageStore(imgAccum, pix, vec4(acc, 1.0));
			vec3 color=clamp(acc,0.0,1.0);
			imageStore(imgOutput, pix, vec4(color, 1.0));
		}
	}

	//avec accumulation temporelle
}




//Si accumulation temporelle -> il faut enlever le break est calcul finalColor dans couleur, il faut enlever le calcul des ombres
//sans accumulation temporelle -> il faut laisser les ombres + le break + enlever le dernier paragraphe dans main
//sans texture blindness -> enlever le calcul des textures dans les couleurs des primitives + mesh + enlever la deuxieme ligne du programme
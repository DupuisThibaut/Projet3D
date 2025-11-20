#version 430 core
// #extension GL_ARB_bindless_texture : require
layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform image2D imgOutput;

uniform vec3 uCamPos;
uniform mat4 uInvViewProj;
uniform ivec2 uResolution;
uniform int nbSphere;
uniform int nbSquare;
uniform int nbLight;
uniform int nbMesh;

#define PI 3.1415926538

vec2 uvTest;
vec2 uvFinal;

// struct intersect{
// 	float t;
// 	vec3 normal;
// 	vec3
// }

struct World{
	mat4 modelMat;
	mat4 invModelMatrix;
	mat3 normalMat;
	vec4 testSphere;
};
layout(std430,binding=9)buffer Worlds{World worlds[];};

struct Sphere{
	vec3 centre; 
	float rayon; 
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	uvec2 text;
	uvec2 padding;
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
	// vec3 n=normalize(ro+rd*t);
	// float theta=atan(n.z,n.x);
	// float phi=acos(n.y);
	// uvTest=vec2((theta+PI)/(2.0*PI),phi/PI);
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
	uvec2 text;
	uvec2 padding;
};
layout(std430,binding=2)buffer Squares{Square squares[];};

float intersectSquare(vec3 ro, vec3 rd, vec3 m_bottom_left, vec3 m_right_vector, vec3 m_up_vector, vec3 m_normal, float lengthUV, float lengthRV){
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

float intersectBVH(vec3 ro, vec3 rd, vec3 minp, vec3 maxp){
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

float intersectMesh(vec3 ro, vec3 rd, int indice){
    int premierVertex=meshes[indice].info[0];
    int premierBVH=meshes[indice].info[1];
    int premierTriangle=meshes[indice].info[2];
    int stack[64];
    int sp=0;
    stack[sp++]=premierBVH;
    float tmin=1e20;
    while(sp>0){
        int node=stack[--sp];
        float tnode=intersectBVH(ro,rd,bvhs[node].minp.xyz,bvhs[node].maxp.xyz);
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
                float t=intersectTriangle(ro,rd,v0,v1,v2);
                if(t>0 && t<tmin){
                    tmin=t;
                    normalTriangleFinal=normalize(cross(v1-v0,v2-v0));
                }
            }
			if(tmin!=1e20)return tmin;
            continue;
        }
        float tLeft=(left!=-1)? intersectBVH(ro,rd,bvhs[left].minp.xyz,bvhs[left].maxp.xyz):-1;
        float tRight=(right!=-1)? intersectBVH(ro,rd,bvhs[right].minp.xyz,bvhs[right].maxp.xyz):-1;
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
        float t=intersectSquare(ro,rd,squares[i].m_bottom_left.xyz,squares[i].m_right_vector.xyz,squares[i].m_up_vector.xyz,squares[i].m_normal.xyz,squares[i].m_up_vector[3],squares[i].m_right_vector[3]);
        if(t>0.0 && t<res.tmin){res.tmin=t;res.hitIndex=i;res.inter=2;uvFinal=uvTest;}
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
			float t=intersectMesh(roLocal,rdLocal,i);
			if(t>0.0){
				vec3 pLocal=roLocal+rdLocal*t;
				vec3 pMonde=(model*vec4(pLocal,1.0)).xyz;
				float t2=length(pMonde-ro);
				if(t2<res.tmin){
					mat3 normalMat=worlds[i].normalMat;
					// mat3 normalMat=transpose(inverse(mat3(model)));
					normalTriangleFinal=normalize(normalMat*normalTriangleFinal);
					// normalTriangleFinal=normalize(mat3(model)*normalTriangleFinal);
					res.tmin=t2;res.hitIndex=i;res.inter=3;
				}
			}
		}
	// }

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

bool testOmbre(vec3 ro, vec3 rd, float dist){
	for (int i=0;i<nbSphere;++i) {
		float t=intersectSphere(ro,rd,spheres[i].centre,spheres[i].rayon);
		if(t>0.0 && t<dist){return true;}
	}
	for (int i=0;i<nbSquare;++i) {
		float t=intersectSquare(ro,rd,squares[i].m_bottom_left.xyz,squares[i].m_right_vector.xyz,squares[i].m_up_vector.xyz,squares[i].m_normal.xyz,squares[i].m_up_vector[3],squares[i].m_right_vector[3]);
		if(t>0.0 && t<dist){return true;}
	}
	for(int i=0;i<nbMesh;i++){
		mat4 model=worlds[i].modelMat;
		mat4 invModelMatrix=worlds[i].invModelMatrix;
		vec3 roLocal=(invModelMatrix*vec4(ro,1.0)).xyz;
		vec3 rdLocal=normalize((invModelMatrix*vec4(rd,0.0)).xyz);
		float t=intersectMesh(roLocal,rdLocal,i);
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
	int nombreRayon=5;
	float pourcentageOmbre=0.0;
	vec3 newRay;
	vec3 ro=p+n*0.01;
	for(int i=0;i<nombreRayon;i++){
		newRay=light+randomInSphere(vec3(pix,i))*lights[0].rayon;
		vec3 L=newRay-p;
		float dist=length(L);			
		vec3 rd=normalize(L);
		if(testOmbre(ro,rd,dist))nombreRayonOmbreDouce++;
	}
	pourcentageOmbre=nombreRayonOmbreDouce/float(nombreRayon);
	return 1.0-pourcentageOmbre;
}

// Vec3 reflect(const Vec3& v, const Vec3& n){
// 	return v - 2*Vec3::dot(v,n)*n;
// }

// Vec3 refract(const Vec3& uv,const Vec3& n, float etai_over_etat) {
// 	float cos_theta = min(Vec3::dot(uv, n),1.0);
// 	Vec3 r_out_perp =   etai_over_etat*(uv + cos_theta*n) ;
// 	Vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.squareLength())) * n;
// 	return (r_out_perp + r_out_parallel);
// }

// Vec3 ComputeRefraction(Ray ray, int NRemainingBounces, Vec3 N, Vec3 intersection, Material material) {
// 	Vec3 unit_dir = ray.direction(); unit_dir.normalize();
// 	Vec3 normal = N;normal.normalize();
// 	float ri;
// 	if (Vec3::dot(unit_dir, normal) >= 0) {
// 		ri = material.index_medium;
// 	} else {
// 		ri = 1./material.index_medium;
// 	}
// 	float cos_theta = min(Vec3::dot((unit_dir*-1.0), normal),1.0);
// 	float sin_theta = std::sqrt(1. - cos_theta * cos_theta);
// 	bool cannot_refract = (ri * sin_theta) > 1.6f;
// 	if (cannot_refract ) {
// 		Vec3 R = reflect(unit_dir,normal);R.normalize();
// 		Ray reflectedRay(intersection + R*0.01f,R);
// 		Vec3 reflectedColor = rayTraceRecursive(reflectedRay,NRemainingBounces-1);
// 		return reflectedColor;
// 	} else {
// 		Vec3 direction = refract(unit_dir, normal, ri);
// 		direction.normalize();
// 		Ray refractedRay(intersection+direction*0.01f, direction);
// 		Vec3 refractedColor = rayTraceRecursive(refractedRay, NRemainingBounces - 1);
// 		return refractedColor;
// 	}
// }

vec3 l=vec3(0.8,0.8,0.8);

vec3 couleurSphere(vec3 ro,vec3 rd,float tmin,int hitIndex,vec2 pix){
	// if(spheres[hitIndex].padding[0]==1){
	// 	sampler2D tex=sampler2D(spheres[hitIndex].text);
	// 	finalColor=texture(tex,uvFinal).rgb;
	// }else{
	// 	finalColor=vec3(1.0,1.0,1.0);
	// }
	vec3 finalColor=vec3(1.0,1.0,1.0);
	vec3 light=lights[0].pos;
	vec3 p=ro+rd*tmin;
	vec3 L=light-p;
	float Ldist=length(L);
	vec3 n=(p-spheres[hitIndex].centre)/spheres[hitIndex].rayon;
	vec3 v=ro-p;
	L=normalize(L);
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
	finalColor*=ombre(p,n,pix,light);
	return finalColor;
}

vec3 couleurSquare(vec3 ro,vec3 rd,float tmin,int hitIndex,vec2 pix){
	// if(squares[hitIndex].padding[0]==1){
	// 	sampler2D tex=sampler2D(squares[hitIndex].text);
	// 	finalColor=texture(tex,uvFinal).rgb;
	// }else{
	// 	finalColor=vec3(1.0,1.0,1.0);
	// }
	vec3 finalColor=vec3(1.0,1.0,1.0);
	vec3 light=lights[0].pos;
	vec3 p=ro+rd*tmin;
	vec3 L=light-p;
	float Ldist=length(L);
	vec3 n=squares[hitIndex].m_normal.xyz;
	vec3 v=ro-p;
	L=normalize(L);
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
	finalColor*=ombre(p,n,pix,light);
	return finalColor;
}

vec3 couleurMesh(vec3 ro,vec3 rd,float tmin,int hitIndex,vec2 pix){
	// if(meshes[hitIndex].padding[0]==1){
	// 	sampler2D tex=sampler2D(meshes[hitIndex].text);
	// 	finalColor=texture(tex,uvFinal).rgb;
	// }else{
	// 	finalColor=vec3(1.0,1.0,1.0);
	// }
	vec3 finalColor=vec3(1.0,1.0,1.0);
	vec3 light=lights[0].pos;
	vec3 p=ro+rd*tmin;
	vec3 L=light-p;
	float Ldist=length(L);
	vec3 n=normalTriangleFinal;
	vec3 v=ro-p;
	L=normalize(L);
	v=normalize(v);
	// n=normalize(n);
	float cosT=max(dot(n,L),0.0);
	vec3 r=reflect(-L,n);
	r=normalize(r);
	float cosA=max(dot(r,v),0.0);
	float shininess=meshes[hitIndex].specular.w;
	vec3 ambient=meshes[hitIndex].ambient.rgb;
	vec3 diffuse=meshes[hitIndex].diffuse.rgb;
	vec3 specular=meshes[hitIndex].specular.rgb;
	finalColor[0]=l[0]*ambient[0]+l[0]*diffuse[0]*cosT+l[0]*specular[0]*pow(cosA,shininess);
	finalColor[1]=l[1]*ambient[1]+l[1]*diffuse[1]*cosT+l[1]*specular[1]*pow(cosA,shininess);
	finalColor[2]=l[2]*ambient[2]+l[2]*diffuse[2]*cosT+l[2]*specular[2]*pow(cosA,shininess);
	finalColor*=ombre(p,n,pix,light);
	return finalColor;
}

vec3 computeRefraction(vec3 ro, vec3 rd, vec3 n, vec3 p, ivec2 pix, float index){
	vec3 finalColor=vec3(1.0,1.0,1.0);
	vec3 unit_dir=rd;
	float ri;
	if(dot(unit_dir,n)>=0){
		ri=index;
	}else{
		ri=1./index;
	}
	float cos_theta=min(dot((unit_dir*-1.0),n),1.0);
	float sin_theta=sqrt(1.-cos_theta*cos_theta);
	bool cannot_refract=(ri*sin_theta)>1.0;
	if(cannot_refract){
		vec3 v=rd-2*dot(rd,n)*n;
		v=normalize(v);
		vec3 vo=p+v*0.01;
		intersection inter=intersectScene(vo,v);
		ro=vo;vec3 rd=v;
		int hitIndex=inter.hitIndex;
		float tmin=inter.tmin;
		int interObjet=inter.inter;
		if(interObjet==1){
			finalColor*=couleurSphere(ro,rd,tmin,hitIndex,pix);
		}else if(interObjet==2){	
			finalColor*=couleurSquare(ro,rd,tmin,hitIndex,pix);
		}else if(interObjet==3){
			finalColor*=couleurMesh(ro,rd,tmin,hitIndex,pix);
		}else{
			finalColor=vec3(0.68,0.85,0.90);
		}
	}else{
		float cos_theta=min(dot(unit_dir,n),1.0);
		vec3 r_out_perp=ri*(unit_dir+cos_theta*n);
		vec3 r_out_parallel=-sqrt(abs(1.0-dot(r_out_perp,r_out_perp)))*n;
		vec3 v=r_out_perp+r_out_parallel;
		v=normalize(v);
		vec3 vo=p+v*0.01;
		intersection inter=intersectScene(vo,v);
		ro=vo;vec3 rd=v;
		int hitIndex=inter.hitIndex;
		float tmin=inter.tmin;
		int interObjet=inter.inter;
		if(interObjet==1){
			finalColor*=couleurSphere(ro,rd,tmin,hitIndex,pix);
		}else if(interObjet==2){	
			finalColor*=couleurSquare(ro,rd,tmin,hitIndex,pix);
		}else if(interObjet==3){
			finalColor*=couleurMesh(ro,rd,tmin,hitIndex,pix);
		}else{
			finalColor=vec3(0.68,0.85,0.90);
		}finalColor=vec3(0.0,1.0,0.0);
	}
	return finalColor;
}

vec3 computeReflection(vec3 ro, vec3 n, vec3 p, ivec2 pix){
	vec3 v=ro-2*dot(ro,n)*n;
	v=normalize(v);
	vec3 vo=p+v*0.01;
	intersection inter=intersectScene(vo,v);
	ro=vo;vec3 rd=v;
	int hitIndex=inter.hitIndex;
	float tmin=inter.tmin;
	int interObjet=inter.inter;
	vec3 finalColor=vec3(1.0,1.0,1.0);
    if(interObjet==1){
		finalColor*=couleurSphere(ro,rd,tmin,hitIndex,pix);
    }else if(interObjet==2){	
		finalColor*=couleurSquare(ro,rd,tmin,hitIndex,pix);
    }else if(interObjet==3){
		finalColor*=couleurMesh(ro,rd,tmin,hitIndex,pix);
    }else{
        finalColor=vec3(0.68,0.85,0.90);
    }
	return finalColor;
}

vec3 couleur(vec3 ro, vec3 rd, intersection inter,ivec2 pix){
	int hitIndex=inter.hitIndex;
	float tmin=inter.tmin;
	int interObjet=inter.inter;
	vec3 finalColor=vec3(1.0,1.0,1.0);
    if(interObjet==1){
		vec3 p=ro+rd*tmin;
		vec3 n=(p-spheres[hitIndex].centre)/spheres[hitIndex].rayon;
		if(spheres[hitIndex].padding[1]==1){
			finalColor*=computeReflection(rd,n,p,pix);
		}
		if(spheres[hitIndex].padding[1]==2){
			finalColor*=computeRefraction(ro,rd,n,p,pix,0.8);
		}
		finalColor*=couleurSphere(ro,rd,tmin,hitIndex,pix);
    }else if(interObjet==2){
		vec3 p=ro+rd*tmin;
		vec3 n=squares[hitIndex].m_normal.xyz;
		if(squares[hitIndex].padding[1]==1){
			finalColor*=computeReflection(rd,n,p,pix);
		}
		finalColor*=couleurSquare(ro,rd,tmin,hitIndex,pix);
    }else if(interObjet==3){
		vec3 p=ro+rd*tmin;
		vec3 n=normalTriangleFinal;
		finalColor*=couleurMesh(ro,rd,tmin,hitIndex,pix);
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
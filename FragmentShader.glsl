#version 410

in vec2 uv;
uniform sampler2D planeTex;
out vec4 color;

#define M_PI 3.141592653589793
#define epsilon 0.0001f
#define saturate(x) clamp(x, 0, 1)

const vec3 eyePosition = vec3(0, 0, 0);
const float fovHalfAngle = M_PI / 8;

const vec3 planePoint = vec3(0, -1, -4);
const vec3 planeNormal = normalize(vec3(0, 1, 1));
const vec3 planeTangent = normalize(vec3(1, 0, 0));
const vec3 planeBitangent = normalize(vec3(0, 1, -1));

const vec3 sphereCenter = vec3(0, 0, -4);
const float sphereRadius = 0.75;

const vec3 beamCenter = vec3(5, 5, 5);
const float beamWidth = 2;
const float beamIntensity = 300;

const vec3 sphereColor = vec3(0.8, 0.8, 0.8);
const vec3 lightColor = vec3(1, 1, 1);

const float sphereReflectivity = 0.8;
const float planeReflectivity = 0.1;

//attenuation components of light
float constant = 1.0;
float linear = 0.027;
float quadratic = 0.0028;

//returns texture coordinates for a given point on the plane
vec2 getPlaneTexCoords(vec3 contactPoint)
{
	vec3 disp = contactPoint - planePoint;
	return 0.2 * vec2(dot(disp, planeTangent), 1 - dot(disp, planeBitangent));
}

//suggested helper functions

bool rayPlaneIntersection(vec3 rayOrigin, vec3 rayDir, out vec3 contactPoint)
{
	//Plane equation D constant value
	float d = dot(planeNormal, planePoint);

	//No intersection happens as the plane and the ray are parallel
	if(dot(planeNormal, rayDir) == 0)
		return false;
	
	float x = (d - dot(planeNormal, rayOrigin)) / dot(planeNormal, rayDir);

	//Intersection Point = rayOrigin + (unit direction vector times common 'x' value between plane and ray)
	contactPoint = rayOrigin + (normalize(rayDir) * x);;
	
	return true;
}

bool raySphereIntersection(vec3 rayOrigin, vec3 rayDir, out vec3 contactPoint, out vec3 contactNormal)
{
	vec3 p = rayOrigin - sphereCenter;
	vec3 d = normalize(rayDir);
	float rSquared = sphereRadius * sphereRadius;
	float p_d = dot(p, normalize(rayDir));

	if(p_d > 0 || dot(p,p) < rSquared)
		return false;

	vec3 a = p - p_d * d;

	float aSquared = dot(a, a);

	if(aSquared > rSquared)
		return false;

	float h = sqrt(rSquared - aSquared);

	vec3 i = a - h * d;

	contactPoint = sphereCenter + i;
	contactNormal = i/sphereRadius;

	return true;
}
vec3 getOneBounceColor(vec3 rayDir, vec3 contactPoint, vec3 contactNormal, vec3 lightPos, bool sphere)
{	
	vec3 result = vec3(0,0,0);
	if(!sphere)
	{
		vec3 beam = normalize(lightPos - contactPoint);
		vec3 reflectedRay = reflect(rayDir, contactNormal);

		float diffuse = max(dot(reflectedRay, beam), 0.0);
		
		float specular = pow(diffuse, planeReflectivity);
		
		result = (diffuse + specular) * lightColor;
	}
	else
	{
		vec3 beam = normalize(lightPos - contactPoint);
		vec3 reflectedRay = reflect(rayDir, contactNormal);

		float diffuse = max(dot(reflectedRay, beam), 0.0);

		float specular = pow(diffuse, sphereReflectivity);
	
		result = (diffuse + specular) * lightColor;
	}
	

	return result;
}

vec4 getTwoBounceColor(vec3 rayDir, vec3 contactPoint, vec3 contactNormal, vec3 lightPos, bool sphere)
{
	vec3 result = vec3(1,1,1);
	vec4 xcolor = vec4(1,1,1,1);
	if(sphere)
	{
		result = getOneBounceColor(rayDir, contactPoint, contactNormal, lightPos, sphere) * sphereColor;
		xcolor = vec4(result, 1.0);
		vec3 reflectedRay = reflect(rayDir, contactNormal);
		vec3 rp;

		//plane texture reflected on sphere
		if(rayPlaneIntersection(contactPoint, reflectedRay, rp))
		{
			float distance = length(rp - contactPoint);
			if(distance < 1.3)
				xcolor = vec4(getOneBounceColor(rayDir, rp, planeNormal, lightPos, false), 1.0) * vec4(lightColor, 1.0) * texture(planeTex, getPlaneTexCoords(rp));
		}
	}
	else
	{
		result = getOneBounceColor(rayDir, contactPoint, contactNormal, lightPos, sphere) * lightColor;
		xcolor = texture(planeTex, getPlaneTexCoords(contactPoint)) * vec4(result, 1.0);
		
		vec3 rs, sn;
		
		//Shadow
		vec3 reflectedRay = lightPos - contactPoint;
		if(raySphereIntersection(contactPoint, reflectedRay, rs, sn))
		{
			xcolor = vec4(0,0,0,1);
		}
		
		//Reflection of sphere in plane
		vec3 reflectedRay2 = reflect(rayDir, contactNormal);
		if(raySphereIntersection(contactPoint, reflectedRay2, rs, sn))
		{
			xcolor = xcolor * vec4(sphereColor, 1.0);;
		}
	}

	return xcolor;
}




void main()
{
	color = vec4(0, 0, 0, 1);
	const float divideBy = 400;
	vec4 xcolor;

	vec3 incomingRayDirection = -normalize(vec3(uv * fovHalfAngle, -1.0));
	
	//TODO

	// rp = ray plane intersection point, rs = ray sphere intersection point, sn = sphere normal at corresponding rs
	vec3 rp, rs, sn;
	vec3 result;

	vec3 lightSource[400];

	vec3 beamLeftTopCorner = vec3(4,5,4);
	float increment = beamWidth/20;
	int m = 0;
	int n = 0;
	for(float i = 0; i < 2; i+=increment )
	{
		for(float j = 0; j < 2; j+=increment)
		{
			lightSource[20 * m + n].x = beamLeftTopCorner.x + i;
			lightSource[20 * m + n].y = beamLeftTopCorner.y;
			lightSource[20 * m + n].z = beamLeftTopCorner.z + j;
			n++;	
		}
		m++;
	}

for(int i = 0; i < 400; i++)
{

	if(rayPlaneIntersection(eyePosition, -incomingRayDirection, rp))
	{
		xcolor = getTwoBounceColor(-incomingRayDirection, rp, planeNormal, lightSource[i], false);		
	}
	
	if(raySphereIntersection(eyePosition, -incomingRayDirection, rs, sn))
	{
		xcolor = getTwoBounceColor(-incomingRayDirection, rs, sn, lightSource[i], true) * vec4(sphereColor, 1.0);
	}
	color+=xcolor;
}
	
	color.rgb = saturate(color.rgb / divideBy);
}
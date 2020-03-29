#version 420 core


out vec4 fragColor;

uniform sampler2D us2dtex;
uniform int mode; // 用來設定是哪個效果
uniform int compare; // 用來判斷是否開啟compare bar
uniform int mouse_x;
uniform int mouse_y;
uniform float time_val;
uniform int bar_x=300;
uniform int screenWidth=600, screenHeight=600;
uniform float time;



// ---------ch7光碟上的code-------------
float sigma_e = 2.0f; 
float sigma_r = 2.8f; 
float phi = 3.4f; 
float tau = 0.99f; 
float twoSigmaESquared = 2.0 * sigma_e * sigma_e;		
float twoSigmaRSquared = 2.0 * sigma_r * sigma_r;		
int halfWidth = int(ceil(2.0 * sigma_r)); 
vec2 img_size = vec2(1024, 611); 
int nbins = 8; 

#define SORT_SIZE  49  
#define MASK_SIZE  7
#define PI 3.141592
//---------

in VertexData
{
	vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
} fs_in;

// 這裡的blur其實是filter
vec4 blur(){
	vec4 color = vec4(0,0,0,0);
	int n = 0;
	int half_size = 3;
	for (int i = -half_size; i <= half_size; ++i) 
	{
		for (int j = -half_size; j <= half_size; ++j) 
		{

			vec4 c = texture(us2dtex, fs_in.texcoord + vec2(i, j) / img_size);
			color += c;
			n++;
		}
	}
	color /= n;
	return color;
}

vec3 first_blur() 
{	
	int blurSize = 3;
	// Texture coordinates are between [0 1], so we map the pixel indices to the [0 1] space
	float dx = 1.0f / screenWidth;
	float dy = 1.0f / screenHeight;

	// To make the blur effect, we average the colors of surrounding pixels
	vec3 sum = vec3(0, 0, 0);
	for(int i = -blurSize; i < blurSize; i++)
		for(int j = -blurSize; j < blurSize; j++)
			sum += texture(us2dtex, fs_in.texcoord + vec2(i * dx, j * dy)).rgb;

	return sum / (4 * blurSize * blurSize);
}

vec3 second_blur(){
	int blurSize = 3;
	// Texture coordinates are between [0 1], so we map the pixel indices to the [0 1] space
	float dx = 1.0f / screenWidth;
	float dy = 1.0f / screenHeight;

	// To make the blur effect, we average the colors of surrounding pixels
	vec3 sum = vec3(0, 0, 0);
	for(int i = -blurSize; i < blurSize; i++)
		for(int j = -blurSize; j < blurSize; j++)
			for(int k = -blurSize; k < blurSize; k++)
				for(int l = -blurSize; l < blurSize; l++)
					sum += texture(us2dtex, fs_in.texcoord + vec2(i * dx, j * dy)).rgb;

	return sum / (4 * blurSize * blurSize);

}


vec3 pixelize() 
{
	int size=6;
	// Texture coordinates are between [0 1], so we map the pixel indices to the [0 1] space
	float dx = 1.0f / screenWidth;
	float dy = 1.0f / screenHeight;

	vec2 baseUV = vec2(0, 0);
	vec2 uvSize = vec2(dx * size, dy * size);

	// To make the pixelization effect, we first cut the texture into many blocks. 
	// All pixels in each block would use the same color which is calculated by averaging all colors in the block

	// Find the base UV. The base UV is the UV on the left-upper corner of the block
	while(fs_in.texcoord.x >= baseUV.x) {
		baseUV.x += uvSize.x;
	}
	while(fs_in.texcoord.y >= baseUV.y) {
		baseUV.y += uvSize.y;
	}
	baseUV -= uvSize;

	// Calcuate the average color for the pixels in each block
	vec3 sum = vec3(0, 0, 0);
	for(int i = 0; i < size; i++)
		for(int j = 0; j < size; j++)
			sum += texture(us2dtex, baseUV + vec2(i * dx, j * dy)).rgb;
	return sum / (size * size);
}




void main(){
	vec4 tex_color = texture(us2dtex ,fs_in.texcoord);

	if (gl_FragCoord.x < bar_x)
	{
		vec4 tex_color = texture(us2dtex ,fs_in.texcoord);
		fragColor = tex_color;
	}
	else if(gl_FragCoord.x >= bar_x && gl_FragCoord.x <= bar_x+10)
	{
		fragColor = vec4(0.0 ,1.0 ,0.0 ,1.0);
	}
	else
	{
	// write final color to the framebuffer
		vec4 result = vec4(0.0);
		if(mode == 0){
			fragColor = tex_color;
		}
		else if(mode == 4)
		{
			vec4 temp = vec4(0.0);
			int iterator = 4;
			for(int i=0;i < iterator;++i)
			{
				result += blur();
			}
			fragColor = result / iterator;
		}
		else if(mode == 5)
		{
			vec3 result = vec3(0.0);
			result = first_blur() + second_blur()/150;
			fragColor = vec4(result ,1.0);
		}
		else if(mode == 6)
		{
			fragColor = vec4(pixelize() ,1.0);
		}
		else if(mode == 7)
		{	
			vec2 uv = fs_in.texcoord;
			//Hint : 
			//Add the sine wave and UV at x axis as distortion
			//Offset = time.
			//fs_in.texcoord.x = fs_in.texcoord.x + power1*sin(fs_in.texcoord.y * power2*PI + offset);
			
			float power1 = 0.01;
			float power2 = 0.1;
			float offset = time;
			
			if( gl_FragCoord.x > screenWidth*3/4)
			{
				float x_position = fs_in.texcoord.x;
				x_position += power1*sin(fs_in.texcoord.y * power2 * PI + offset);
				fragColor = texture(us2dtex ,vec2(x_position ,fs_in.texcoord.y));
			}
			else if(gl_FragCoord.x <= screenWidth*3/4)
			{
				fragColor = texture(us2dtex ,fs_in.texcoord);
			}
			if(gl_FragCoord.y <= screenHeight*3/4)
			{
				fragColor = texture(us2dtex ,fs_in.texcoord);
			}
			else if(gl_FragCoord.y > screenHeight*3/4)
			{
				float y_position = fs_in.texcoord.y;
				y_position += power1*sin(fs_in.texcoord.x * power2 * PI + offset);
				fragColor = texture(us2dtex ,vec2( fs_in.texcoord.x,y_position));
			}
		}
	}
}
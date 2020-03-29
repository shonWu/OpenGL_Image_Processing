#include "../Externals/Include/Include.h"

#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define MENU_ORIGANAL 4
#define MENU_NORMAL 5
#define MENU_ABSTRACTION 6
#define MENU_WATERCOLOR 7
#define MENU_BLOOM_EFFECT 8
#define MENU_PIXEILIZE 9 
#define MENU_SINE_WAVE 10

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

using namespace glm;
using namespace std;

mat4 view;					// V of MVP, viewing matrix
mat4 proj;			// P of MVP, projection matrix
mat4 model;					// M of MVP, model matrix


vec3 eye = vec3(0.3, 0.95, 0.4); //相機在世界座標的位置
vec3 center = vec3(0.3, 0.95, 0.5); // 相機對準物體在世界座標的位置
vec3 up = vec3(0.0, 0.95, 0.0); // 相機向上的方向在世界座標的位置
vec3 temp = vec3(0.0, 0.0, 0.0);

GLint um4p;
GLint um4mv;
GLint us2dtex;
GLuint program;			// shader program id


// FBO
GLuint program2;
GLuint vao2;
GLuint window_vertex_buffer;
GLuint FBO;
GLuint depthRBO;
GLuint FBODataTexture;
GLuint time;

//Image Processing
GLuint program_bar_x;
GLuint program2_bar_x;
// 0: orignal,1: normal,2: Image Abstraction, 3: watercolor
GLuint program_mode; 
// 4: watercolor
GLuint program2_mode;
GLuint program2_width;
GLuint program2_height;


int mouse_x;
int mouse_y;


// 輸入window與texture的對應位置
static const GLfloat window_vertex[] =
{
	//vec2 position vec2 texture_coord
	1.0f, -1.0f, 1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f, 0.0f,
	-1.0f, 1.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f
};


using namespace glm;
using namespace std;

char** loadShaderSource(const char* file)
{
    FILE* fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *src = new char[sz + 1];
    fread(src, sizeof(char), sz, fp);
    src[sz] = '\0';
    char **srcp = new char*[1];
    srcp[0] = src;
    return srcp;
}

void freeShaderSource(char** srcp)
{
    delete[] srcp[0];
    delete[] srcp;
}

// define a simple data structure for storing texture image raw data
typedef struct _TextureData
{
    _TextureData(void) :
        width(0),
        height(0),
        data(0)
    {
    }

    int width;
    int height;
    unsigned char* data;
} TextureData;

// load a png image and return a TextureData structure with raw data
// not limited to png format. works with any image format that is RGBA-32bit
TextureData loadImage(const char* Filepath)
{
	TextureData texture;
	int n;
	stbi_set_flip_vertically_on_load(true);
	stbi_uc *data = stbi_load(Filepath, &texture.width, &texture.height, &n, 4);
	if (data != NULL)
	{
		texture.data = new unsigned char[texture.width * texture.height * 4 * sizeof(unsigned char)];
		memcpy(texture.data, data, texture.width * texture.height * 4 * sizeof(unsigned char));
		stbi_image_free(data);
	}
	return texture;
}

// define data structure
struct Material
{
	GLuint diffuse_tex;
};
/*
typedef struct
{
	GLuint vao;			// vertex array object
	GLuint vbo;			// vertex buffer object
	GLuint vboTex;		// vertex buffer object of texture
	GLuint ebo;

	GLuint p_normal;
	int materialId;
	int indexCount;
	GLuint m_texture;
} Shape;
*/

typedef struct 
{
	GLuint vao;
	GLuint vbo_position;
	GLuint vbo_normal;
	GLuint vbo_texcoord;
	GLuint ibo;
	int drawCount;
	int materialID; // 用途?? 用來查找m_material的內容
	Material material_log; // 用途??
}Shape;


vector<Shape> m_shape;
vector<Material> m_material;
/**/
void my_InitScene() {
	

	glClearColor(0.0f, 0.6f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	program = glCreateProgram();

	GLuint vs_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs_shader = glCreateShader(GL_FRAGMENT_SHADER);

	char ** vs_source = loadShaderSource("vertex.vs.glsl");
	char ** fs_source = loadShaderSource("fragment.fs.glsl");

	glShaderSource(vs_shader, 1, vs_source, NULL);
	glShaderSource(fs_shader, 1, fs_source, NULL);

	freeShaderSource(vs_source);
	freeShaderSource(fs_source);

	glCompileShader(vs_shader);
	glCompileShader(fs_shader);


	glAttachShader(program, vs_shader);
	glAttachShader(program, fs_shader);
	glLinkProgram(program);
	glUseProgram(program);

	um4mv = glGetUniformLocation(program, "um4mv");
	um4p = glGetUniformLocation(program, "um4p");
	us2dtex = glGetUniformLocation(program, "us2dtex");
	program_mode = glGetUniformLocation(program, "mode");
	program_bar_x = glGetUniformLocation(program, "bar_x");





	const aiScene *scene =
		aiImportFile("sponza.obj", aiProcessPreset_TargetRealtime_MaxQuality);

	// texture
	cout << "load material...." << endl;
	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		aiMaterial *sc_material = scene->mMaterials[i];
		Material material;
		aiString texturePath;
		// 如果load成功
		if (sc_material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS)
		{
			TextureData tdata = loadImage(texturePath.C_Str());
			// load width, height and data from texturePath.C_Str();
			glGenTextures(1, &material.diffuse_tex);
			glBindTexture(GL_TEXTURE_2D, material.diffuse_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
			glGenerateMipmap(GL_TEXTURE_2D); //產生mipmap的效果
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		else
		{
			// load some default image as default_diffuse_tex
			TextureData tdata = loadImage("vrata_kr.JPG");
			// load width, height and data from texturePath.C_Str();
			glGenTextures(1, &material.diffuse_tex);
			glBindTexture(GL_TEXTURE_2D, material.diffuse_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
			// Mipmap會預先產生縮小的圖案，需要時會就被拿來使用
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		// save material…
		m_material.push_back(material);
	}

	// meshes are composed by position and face
	cout << "load meshs...." << endl;
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		aiMesh *mesh = scene->mMeshes[i];
		Shape shape;

		glGenVertexArrays(1, &shape.vao);
		glBindVertexArray(shape.vao);

		// create 3 vbos to hold data
		vector<float> positions;
		vector<float> normals;
		vector<float> texcoords;

		for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
		{
			positions.push_back(mesh->mVertices[v][0]);
			positions.push_back(mesh->mVertices[v][1]);
			positions.push_back(mesh->mVertices[v][2]);
			normals.push_back(mesh->mNormals[v][0]);
			normals.push_back(mesh->mNormals[v][1]);
			normals.push_back(mesh->mNormals[v][2]);
			texcoords.push_back(mesh->mTextureCoords[0][v][0]);
			texcoords.push_back(mesh->mTextureCoords[0][v][1]);
			// mesh->mVertices[v][0~2] => position
			// mesh->mNormals[v][0~2] => normal
			// mesh->mTextureCoords[0][v][0~1] => texcoord
		}

		// create 1 ibo to hold data
		vector<int> indices;
		// Face的意義?? 
		// 表示三角形的正面與反面，一般來說逆時鐘拇指筆的方向為正面
		for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
		{
			indices.push_back(mesh->mFaces[f].mIndices[0]);
			indices.push_back(mesh->mFaces[f].mIndices[1]);
			indices.push_back(mesh->mFaces[f].mIndices[2]);
			// mesh->mFaces[f].mIndices[0~2] => index
		}
		// glVertexAttribPointer / glEnableVertexArray calls…
		shape.materialID = mesh->mMaterialIndex;
		shape.drawCount = mesh->mNumFaces * 3;

		// position buffers
		glGenBuffers(1, &shape.vbo_position);
		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_position);
		glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), &positions[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		// normal buffers
		glGenBuffers(1, &shape.vbo_normal);
		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_normal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);

		// textures coordation buffers
		glGenBuffers(1, &shape.vbo_texcoord);
		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_texcoord);
		glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(float), &texcoords[0], GL_STATIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		// index buffers
		glGenBuffers(1, &shape.ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		shape.materialID = mesh->mMaterialIndex;
		shape.drawCount = mesh->mNumFaces * 3;
		// save shape
		m_shape.push_back(shape);

		glBindVertexArray(0);
	}
	aiReleaseImport(scene);
}


// 大致上來說沒問題，不過在test proj要加入mouse_x
void my_InitFBO() {
	glUseProgram(0);
	// FBO program
	program2 = glCreateProgram();

	GLuint vs_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs_shader = glCreateShader(GL_FRAGMENT_SHADER);

	char ** vs_source = loadShaderSource("vertex2.vs.glsl");
	char ** fs_source = loadShaderSource("fragment2.fs.glsl");

	glShaderSource(vs_shader, 1, vs_source, NULL);
	glShaderSource(fs_shader, 1, fs_source, NULL);

	freeShaderSource(vs_source);
	freeShaderSource(fs_source);

	glCompileShader(vs_shader);
	glCompileShader(fs_shader);


	glAttachShader(program2, vs_shader);
	glAttachShader(program2, fs_shader);
	glLinkProgram(program2);
	glUseProgram(program2);

	glClearColor(0.0f, 0.6f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	
	program2_mode = glGetUniformLocation(program2, "mode");
	program2_bar_x = glGetUniformLocation(program2, "bar_x");
	program2_width = glGetUniformLocation(program2, "screenWidth");
	program2_height = glGetUniformLocation(program, "screenHeight");
	time = glGetUniformLocation(program2, "time");
	

	// Init FBO
	glGenVertexArrays(1, &vao2);
	glBindVertexArray(vao2);

	glGenBuffers(1, &window_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, window_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(window_vertex), window_vertex, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, (const GLvoid*)(sizeof(GL_FLOAT) * 2));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glGenFramebuffers(1, &FBO);

}


void My_Init()
{
	const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	cout << version << endl;

	glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	
	my_InitScene();
	my_InitFBO();

	glUseProgram(program);
}


void My_Display()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static const GLfloat green[] = { 0.0f, 0.25f, 0.0f, 1.0f };
	static const GLfloat one = 1.0f;
	glClearBufferfv(GL_COLOR, 0, green);
	glClearBufferfv(GL_DEPTH, 0, &one);

	
	
	model = translate(mat4(),temp);
	view = lookAt(eye, center, up);
	// 更新uniform的資料
	glUseProgram(program);
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(proj));
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(us2dtex, 0);

	// 清除資料再render
	glClearBufferfv(GL_COLOR, 0, green);
	glClearBufferfv(GL_DEPTH, 0, &one);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// 一般的render
	for (int i = 0; i < m_shape.size(); ++i)
	{
		glBindVertexArray(m_shape[i].vao);
		int materialID = m_shape[i].materialID;
		glBindTexture(GL_TEXTURE_2D, m_material[materialID].diffuse_tex);
		glDrawElements(GL_TRIANGLES, m_shape[i].drawCount, GL_UNSIGNED_INT, 0);
	}
	// 結束綁定，並且清除資料
	glBindFramebuffer(GL_FRAMEBUFFER, 0); 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, FBODataTexture);
	glBindVertexArray(vao2);
	glUseProgram(program2);
	glUniform1f(time, timer_cnt);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // 將在framebuffer的影像輸出


	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glUseProgram(program);
	glUniform1i(program_bar_x, width / 2);
	glUseProgram(0);

	glUseProgram(program2);
	glUniform1i(program2_bar_x, width / 2);
	glUniform1i(program2_width, width);
	glUniform1i(program2_height, height);
	glUseProgram(0);

	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;

	// perspective(fov, aspect_ratio, near_plane_distance, far_plane_distance)
	// ps. fov = field of view, it represent how much range(degree) is this camera could see 
	proj = perspective(radians(60.0f), viewportAspect, 0.1f, 1000.0f);

	// lookAt(camera_position, camera_viewing_vector, up_vector)
	// up_vector represent the vector which define the direction of 'up'
	view = lookAt(vec3(-10.0f, 5.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)); 

	//Delete old Depth RBO and create new Depth RBO
	glDeleteRenderbuffers(1, &depthRBO);
	glDeleteTextures(1, &FBODataTexture);
	glGenRenderbuffers(1, &depthRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32,width, height);

	// Create texture FBO
	glGenTextures(1, &FBODataTexture);
	glBindTexture(GL_TEXTURE_2D, FBODataTexture);
	// set texture data(??) or clean texture data 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); // data直接使用null??
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Set depth FBO and texture FBO to current FBO
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBODataTexture, 0);
}

void My_Timer(int val)
{	
	timer_cnt++;
	glutPostRedisplay();
	if (timer_enabled) 
	{
		glutTimerFunc(timer_speed, My_Timer, val);
	}
}

void My_Mouse(int button, int state, int x, int y)
{
	if(state == GLUT_DOWN)
	{
		printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
		mouse_x = x;
		mouse_y = y;
	}
	else if(state == GLUT_UP)
	{
		printf("Mouse %d is released at (%d, %d)\n", button, x, y);
	}
}

void My_Keyboard(unsigned char key, int x, int y)
{
	printf("Key %c is pressed at (%d, %d)\n", key, x, y);
	if (key == 's')
	{
		temp = temp + vec3(0, 0, 0.01);
	}
	else if (key == 'w')
	{
		temp = temp - vec3(0, 0, 0.01);
	}
	else if (key == 'd')
	{
		temp = temp + vec3(0.01, 0, 0);
	}
	else if (key == 'a')
	{
		temp = temp - vec3(0.01, 0, 0);
	}
	else if (key == 'q')
	{
		temp = temp - vec3(0, 0.01, 0);
	}
	else if (key == 'e')
	{
		temp = temp + vec3(0, 0.01, 0);
	}
}

void My_SpecialKeys(int key, int x, int y)
{
	switch(key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	switch(id)
	{
	case MENU_TIMER_START:
		if(!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	case MENU_ORIGANAL:
		cout << "use orginal texture" << endl;
		glUseProgram(program);
		glUniform1i(program_mode, 0);
		glUseProgram(program2);
		glUniform1i(program2_mode, 0);
		break;
	case MENU_NORMAL:
		cout << "use normal effect" << endl;
		glUseProgram(program); //  不能沒有這行
		glUniform1i(program_mode, 1);
		glUseProgram(program2);
		glUniform1i(program2_mode, 0);
		break;
	case MENU_ABSTRACTION:
		cout << "use abstraction effect" << endl;
		glUseProgram(program); //  不能沒有這行
		glUniform1i(program_mode, 2);
		glUseProgram(program2);
		glUniform1i(program2_mode, 0);
		break;
	case MENU_WATERCOLOR:
		cout << "use water color effect" << endl;
		glUseProgram(program); //  不能沒有這行
		glUniform1i(program_mode, 0);
		glUseProgram(program2);
		glUniform1i(program2_mode, 4);
		break;
	case MENU_BLOOM_EFFECT:
		cout << "use water bloom effect" << endl;
		glUseProgram(program); //  不能沒有這行
		glUniform1i(program_mode, 0);
		glUseProgram(program2);
		glUniform1i(program2_mode, 5);
		break;
	case MENU_PIXEILIZE:
		cout << "use pixelize effect" << endl;
		glUseProgram(program); //  不能沒有這行
		glUniform1i(program_mode, 0);
		glUseProgram(program2);
		glUniform1i(program2_mode, 6);
		break;
	case MENU_SINE_WAVE:
		glUseProgram(program); //  不能沒有這行
		glUniform1i(program_mode, 0);
		glUseProgram(program2);
		glUniform1i(program2_mode, 7);
		break;
	default:
		break;
	}
}


// ??不太懂
void my_Motion(int x, int y) {
	int diff_x = x - mouse_x;
	int diff_y = y - mouse_y;
	mouse_x = x;
	mouse_y = y;

	mat3 rotY = mat3(cos(diff_x / 400.0), 0.0, sin(diff_x / 400.0),
		0.0, 1.0, 0.0,
		-sin(diff_x / 400.0), 0.0, cos(diff_x / 400.0));
	mat3 rotX = mat3(1.0, 0.0, 0.0,
		0.0, cos(diff_y / 400.0), -sin(diff_y / 400.0),
		0.0, sin(diff_y / 400.0), cos(diff_y / 400.0));

	center = center - eye;
	center = rotY * rotX * center;
	center = center + eye;
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
    // Change working directory to source code path
    chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(600, 600);
	glutCreateWindow("AS2_Framework"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
    glPrintContextInfo();
	My_Init();


	// Create a menu and bind it to mouse right button.
	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddMenuEntry("Exit", MENU_EXIT);
	glutAddMenuEntry("Orignal", MENU_ORIGANAL);
	glutAddMenuEntry("Normal", MENU_NORMAL);
	glutAddMenuEntry("Abstraction", MENU_ABSTRACTION);
	glutAddMenuEntry("WaterColor", MENU_WATERCOLOR);
	glutAddMenuEntry("Bloom effect", MENU_BLOOM_EFFECT);
	glutAddMenuEntry("Pixelizate", MENU_PIXEILIZE);
	glutAddMenuEntry("sine wave", MENU_SINE_WAVE);
	

	

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutMotionFunc(my_Motion);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0); 

	// Enter main event loop.
	glutMainLoop();

	return 0;
}

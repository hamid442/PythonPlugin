/********************************************************************************
Copyright (C) 2014 Andrew Skinner <obs@theandyroid.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
********************************************************************************/

#include <Python.h>

#include "structmember.h"

#include "CppImageSource.h"
#include "PythonPlugin.h"
#include "PyImageSource.h"
#include "PyVect2.h"



CppImageSource::CppImageSource(XElement *data)
{

	this->data = data;
	Log(TEXT("Python Source Constructor"));

	PythonPlugin *pyPlug = PythonPlugin::instance;
	if (pyPlug == NULL){
		Log(TEXT("Python instance Does not exist"));
	}

	
	//Set a safe image size
	imageSize = Vect2(500, 500);

	pyImgSrc = NULL;	
	
}

CppImageSource::~CppImageSource(){	



	
	PyGILState_STATE gstate;
	gstate = PyGILState_Ensure();
	

	//remove hotkeys
	for (hotkey_map::iterator it = hotkeyToCallable.begin(); it != hotkeyToCallable.end(); ++it){
		OBSDeleteHotkey(it->first);
		PyObject* callback = it->second;
		Py_DECREF(callback);
	}

	hotkeyToCallable.clear();


		
	if (pyImgSrc != NULL){
		CallPythonFunction("Destructor");
		pyImageSource* tmp = (pyImageSource*)pyImgSrc;
		tmp->cppImageSource = NULL;
		Py_XDECREF(pyImgSrc);
	}				
	
	
	
	pyImgSrc = NULL;
	Log(TEXT("Python Source Destructor"));
	

	if (texture) {
		delete texture;
		texture = nullptr;
	}
	PyGILState_Release(gstate);
	

}


void CppImageSource::Tick(float seconds){

	PythonPlugin *pyPlug = PythonPlugin::instance;


	PyGILState_STATE gstate;
	gstate = PyGILState_Ensure();

	PyObject *pyTick = PyObject_GetAttrString(pyImgSrc, (char*) "Tick");
	if (pyTick != NULL && PyCallable_Check(pyTick)){

	}
	else{
		Py_XDECREF(pyTick);
		Log(TEXT("tick function not callable"));
		PyGILState_Release(gstate);
		return;
	}

	
	PyObject *argList = Py_BuildValue("(f)", seconds);
	PyObject *result = PyObject_CallObject(pyTick, argList);

	if (pyHasError()){
		PyGILState_Release(gstate);
		return;
	}

	Py_DECREF(argList);
	Py_DECREF(pyTick);
	Py_DECREF(result);


	PyGILState_Release(gstate);

}



void CppImageSource::Render(const Vect2 &pos, const Vect2 &size){
	
	renderSize.x = size.x;  //cx
	renderSize.y = size.y;  //cy

	


	if (!texture) {
		//Texture is created when Python calls SetBuffers()
		return;
	}
		

	PyGILState_STATE gstate;
	gstate = PyGILState_Ensure();

	PyObject *pyRender = PyObject_GetAttrString(pyImgSrc, (char*) "Render");
	if (pyRender == NULL && !PyCallable_Check(pyRender)){
		Log(TEXT("Render not callable"));
		PyGILState_Release(gstate);
		return;
	}

	PyObject * OBSModule = PyImport_ImportModule("OBS");
	if (OBSModule == NULL){
		Log(TEXT("OBSModule null"));
	}
	PyObject * vect2 = PyObject_GetAttr(OBSModule, PyUnicode_FromString("Vect2"));
	if (vect2 == NULL){
		Log(TEXT("vect2 null"));
	}
	
	PyObject *argList = Py_BuildValue("(dd)", size.x, size.y);
	PyObject *pySize = PyObject_CallObject(vect2, argList);
	if (pySize == NULL){
		Log(TEXT("obj null"));
		
		PyGILState_Release(gstate);
	
		return;
	}


	Py_DECREF(argList);
	argList = Py_BuildValue("(dd)", pos.x, pos.y);
	PyObject *pyPos = PyObject_CallObject(vect2, argList);
	if (pyPos == NULL){
		Log(TEXT("obj null"));
		PyGILState_Release(gstate);
		
		return;
	}
	Py_DECREF(argList);
	Py_DECREF(vect2);

	argList = Py_BuildValue("(OO)", pyPos, pySize);
	PyObject *result = PyObject_CallObject(pyRender, argList);
	
	if (pyHasError()){
		PyGILState_Release(gstate);
		return;
	}

	
	
	Py_DECREF(OBSModule);
	Py_DECREF(pySize);
	Py_DECREF(pyPos);
	
	Py_DECREF(argList);
	Py_DECREF(result);
	Py_DECREF(pyRender);
	

	PyGILState_Release(gstate);
	

	return;
}


void *CppImageSource::getBackBuffer(){
	if (isDoubleBuffer == true){
		if (pixelFront == pixelA){
			return pixelB;
		}
		else{
			return pixelA;
		}
	}else{
		return pixelA;
	}	
}

void *CppImageSource::getFrontBuffer(){
	if (pixelFront == pixelA){
		return pixelA;
	}
	else{
		return pixelB;
	}

}

void CppImageSource::flipPixelBuffers(){
	if (pixelFront == pixelA && isDoubleBuffer == true){
		pixelFront = pixelB;
	}
	else{
		pixelFront = pixelA;
	}

}



 Vect2 CppImageSource::GetSize() const{
	//This is the size of the texture 
	 return  imageSize;
}

 int CppImageSource::getHeight(){
	 return imageSize.y;
 }
 int CppImageSource::getWidth(){
	 return imageSize.x;
 }


 void CppImageSource::setHeight(int y){

	 imageSize.y = y;

	//Need to update the texture.
	 texture = MakeTexture();
 }

 void CppImageSource::ColourArea(void* pxdata){
	 for (int i = 0; i < texture->Width() * 4 * texture->Height(); i += 4){
		 BYTE* data = ((UCHAR*)pxdata);
		 data[i + 0] = 0xff; //Red
		 data[i + 1] = 0xff; //Green
		 data[i + 2] = 0xff; //Blue
		 data[i + 3] = 0xff; //Alpha
	 }
 }


 void CppImageSource::setWidth(int x){
	//need to update the texture.
		
	 imageSize.x = x;

	 //Need to update the texture.
	 texture = MakeTexture();
 }


 Texture* CppImageSource::MakeTexture(){
	 if (texture != NULL){
		 delete texture;  //free memory
	}
	 texture = CreateTexture(imageSize.x, imageSize.y, texFormat, nullptr, FALSE, FALSE);
	 return texture;
 }
 

 void CppImageSource::ClearTexture(void *pxdata)
 {
	 BYTE *lpData;
	 UINT pitch;
	 if (texture) {
		 texture->Map(lpData, pitch);
		 memset(lpData, 0, pitch * texture->Height());
		 memset(pxdata, 0, (imageSize.x * imgDepth) * imageSize.y);
		 texture->Unmap();
	 }
 }

 void CppImageSource::setupDoubleBuffers(void *A, void *B,long width, long height){
	 this->pixelA = A;
	 this->pixelB = B;
	 this->pixelFront = this->pixelA;
	 this->isDoubleBuffer = true;
	 imageSize.x = width;
	 imageSize.y = height;
	 this->texture = MakeTexture();

 }



 void CppImageSource::setupSingleBuffer(void *A, long width,long height){
	 this->pixelA = A;
	 this->pixelB = A;
	 this->pixelFront = this->pixelA;
	 this->isDoubleBuffer = false;
	 imageSize.x = width;
	 imageSize.y = height;
	 this->texture = MakeTexture();
 }


 bool CppImageSource::setupFormats(char* format){
	 if (strcmp(format, "A") == 0){
		 texFormat = GS_ALPHA;
		 imgFormat = GS_IMAGEFORMAT_A8;
		 imgDepth = 1;
	 }
	
	 else if (strcmp(format, "L") == 0){
		 texFormat = GS_GRAYSCALE;
		 imgFormat = GS_IMAGEFORMAT_L8;
		 imgDepth = 1;
	 }
	 else if (strcmp(format, "RGB") == 0){

		 texFormat = GS_RGB;
		 imgFormat = GS_IMAGEFORMAT_RGB;
		 imgDepth = 3;
	 }
	 else if (strcmp(format, "RGBA") == 0){

		 texFormat = GS_RGBA;
		 imgFormat = GS_IMAGEFORMAT_RGBA;
		 imgDepth = 4;
	 }
	 else if (strcmp(format, "BGR") == 0){
		 texFormat = GS_BGR;
		 imgFormat = GS_IMAGEFORMAT_BGR;
		 imgDepth = 3;
	 }
	 else if (strcmp(format, "BGRA") == 0){
		 texFormat = GS_BGRA;
		 imgFormat = GS_IMAGEFORMAT_BGRA;
		 imgDepth = 4;
	 }
	 else{
		 return false;	 
	 }

	 return true;
 }

 
 
 void CppImageSource::BeginScene(){ 
	 PyGILState_STATE gstate;
	 gstate = PyGILState_Ensure();
	 CallPythonFunction("BeginScene");
	 PyGILState_Release(gstate);
 }


 void CppImageSource::EndScene(){
	 PyGILState_STATE gstate;
	 gstate = PyGILState_Ensure();
	 CallPythonFunction("EndScene");
	 PyGILState_Release(gstate);
 }

 void CppImageSource::GlobalSourceEnterScene(){	 
	 PyGILState_STATE gstate;
	 gstate = PyGILState_Ensure();
	 CallPythonFunction("GlobalSourceEnterScene");
	 PyGILState_Release(gstate);
 }


 void CppImageSource::GlobalSourceLeaveScene(){
	 PyGILState_STATE gstate;
	 gstate = PyGILState_Ensure();
	 CallPythonFunction("GlobalSourceLeaveScene");
	 PyGILState_Release(gstate);
 }


 void CppImageSource::ChangeScene(){
	 PyGILState_STATE gstate;
	 gstate = PyGILState_Ensure();
	 CallPythonFunction("ChangeScene");
	 PyGILState_Release(gstate);
 }

 void CppImageSource::UpdateSettings(){
	 
	 PyGILState_STATE gstate;
	 gstate = PyGILState_Ensure();
	 CallPythonFunction("UpdateSettings");
	 PyGILState_Release(gstate);
	 
	 
 }



 void CppImageSource::CallPythonFunction(char* funcName){

	 PythonPlugin *pyPlug = PythonPlugin::instance;
	

	 PyObject *pyFunc = PyObject_GetAttrString(pyImgSrc, funcName);
	 if (pyFunc != NULL && PyCallable_Check(pyFunc)){

	 }
	 else{
		 Py_XDECREF(pyFunc);
		 Log(TEXT("%s not callable",funcName));
		 return;
	 }

	 PyObject * OBSModule = PyImport_ImportModule("OBS");
	 if (OBSModule == NULL){
		 Log(TEXT("OBSModule null"));
	 }

	 PyObject *argList = Py_BuildValue("()");
	 PyObject *ret = PyObject_CallObject(pyFunc, argList);

	 pyHasError();
	 Py_XDECREF(ret);
	 Py_XDECREF(argList);	 
	 Py_XDECREF(OBSModule);
	 Py_XDECREF(pyFunc);
	 

	 
 }


 String CppImageSource::getSourceName(){
	 String global = data->GetParent()->GetParent()->GetName();
	 bool bglobal = false;
	 if (global.Compare(String("global sources"))){
		 bglobal = true;
	 }

	 Scene *scene = OBSGetScene(); //Gets current scene
	 String name = "";
	 if (!bglobal){
		 for (int i = 0; i < scene->NumSceneItems(); i++){
			 SceneItem *sceneItem = scene->GetSceneItem(i);
			 if (sceneItem->GetSource() == this){
				 name = String(sceneItem->GetName());
				 break;
			 }
		 }
	 }
	 else{
		//Global sources are stupid
	 }

	 return name;
 }

 String CppImageSource::getSceneName(){ 
	return	 String(OBSGetSceneName()); 
 }
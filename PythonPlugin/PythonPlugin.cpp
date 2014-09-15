#include "PythonPlugin.h"

#include <windowsx.h>
#include <cstdio>
#include <windows.h>
#include <cstdlib>
#include "PyOBS.h"
#include "PyXElement.h"





#define VIDEO_SOURCE_CLASS TEXT("PythonSource")

PythonPlugin *PythonPlugin::instance = NULL;



bool STDCALL ConfigureVideoSource(XElement *element, bool bCreating)
{


	Log(TEXT("Python Source Configure"));
	XElement *dataElement = element->GetElement(TEXT("data"));


	bool isMissingDataElement;
	if (isMissingDataElement = !dataElement) {
		dataElement = element->CreateElement(TEXT("data"));
	}

	

	/*Called when the element is created at stream start.*/

	//This is called when the source goes live


	//Get the plugin instance
	PythonPlugin *pyPlug = PythonPlugin::instance;
	if (pyPlug == NULL){
		Log(TEXT("Python instance Does not exist"));
		return NULL;
	}


	/*
	DWORD dwWaitResult = WaitForSingleObject(
		pyPlug->ghMutex,    // handle to mutex
		INFINITE);  // no time-out interval
	*/



	PyGILState_STATE gstate;
	gstate = PyGILState_Ensure();

	//Python
	PyObject *pName, *pModule, *pFunc;


	pName = PyString_FromString("selectionGUI");
	//pName = PyString_FromString("baseGUI");
	pyHasError();


	PyObject *dict = PyImport_GetModuleDict();
	if (!PyDict_Contains(dict, pName)){
		pModule = PyImport_Import(pName);
	}
	else{
		pModule = PyDict_GetItem(dict, pName);
		pModule = PyImport_ReloadModule(pModule);
	}


	
	if (pyHasError()){
		Py_XDECREF(pName);
		return false;
	}


	if (pModule != NULL) {
		pFunc = PyObject_GetAttrString(pModule, (char*) "gui");
		//pFunc = PyObject_GetAttrString(pModule, (char*) "guimain");

		if (pFunc && PyCallable_Check(pFunc)) {
			PyObject *argList = Py_BuildValue("");
			PyObject *pyConfig = PyObject_CallObject((PyObject*)&PyXElement_Object, NULL);
			Py_XDECREF(argList);
			((PyXElement*)pyConfig)->element = dataElement;

			argList = Py_BuildValue("(O)", pyConfig);
			PyObject_CallObject(pFunc, argList);			

			if (pyHasError()){
				Py_XDECREF(pModule);
				Py_XDECREF(pName);
				Py_XDECREF(pyConfig);
				PyGILState_Release(gstate);
				return false;
			}
			Py_XDECREF(pyConfig);


		}
		else {


		}
		
		Py_DECREF(pFunc);
	}
	else {
		PyErr_Print();


	}

	Py_XDECREF(pModule);
	Py_XDECREF(pName);
	

	PyGILState_Release(gstate);

	//ReleaseMutex(pyPlug->ghMutex);
	return true;
}

ImageSource* STDCALL CreatePythonSource(XElement *data)
{

	Log(TEXT("Python Create ImageSource"));


	/*Called when the element is created at stream start.*/

	//This is called when the source goes live


	//Get the plugin instance
	PythonPlugin *pyPlug = PythonPlugin::instance;
	if (pyPlug == NULL){
		Log(TEXT("Python instance Does not exist"));	
		return NULL;
	}	

	/*
	DWORD dwWaitResult = WaitForSingleObject(
		pyPlug->ghMutex,    // handle to mutex
		INFINITE);  // no time-out interval
	*/
	
	
	PyGILState_STATE gstate;
	gstate = PyGILState_Ensure();	

	
	
	//Create Image source
	pyPlug->pImageSource = new CppImageSource(data);	

	
	//Python
	PyObject *pName, *pModule, *pFunc;
	pyImageSource *pyImgSrc;

	pName = PyString_FromString("playvideoClass");

	pyHasError();
	pModule = PyImport_Import(pName);
	pyHasError();
	bool pyObjectCreated = false;

	if (pModule != NULL) {
		pFunc = PyObject_GetAttrString(pModule, (char*) "playVideo");
		pyHasError();
		// pFunc is a new reference 
		if (pFunc && PyCallable_Check(pFunc)) {
			//pass in config
			PyObject *argList = Py_BuildValue("");
			PyObject *pyConfig = PyObject_CallObject((PyObject*)&PyXElement_Object, NULL);
			Py_XDECREF(argList);
			((PyXElement*)pyConfig)->element = data;
			argList = Py_BuildValue("(O)",pyConfig);
			pyImgSrc = (pyImageSource*)PyObject_CallObject(pFunc, argList);
			if (!pyHasError()){
				pyImgSrc->cppImageSource = pyPlug->pImageSource;
				pyPlug->pImageSource->pyImgSrc = (PyObject*)pyImgSrc;
				pyObjectCreated = true;
			}
			Py_XDECREF(argList);

		}
		else {			
		}
		
		Py_XDECREF(pFunc);
	}
	else {
		PyErr_Print();
	}

	
	Py_XDECREF(pModule);
	Py_XDECREF(pName);
	
	


	PyGILState_Release(gstate);

	//ReleaseMutex(pyPlug->ghMutex);
	if (pyObjectCreated){
		return pyPlug->pImageSource;
	}
	else{
		return NULL;
	}
}



PythonPlugin::PythonPlugin()
{
	isDynamicLocale = false;
	Log(TEXT("Python plugin loaded"));
	if (!locale->HasLookup(KEY("PluginName"))) {
		isDynamicLocale = true;
		int localizationStringCount = sizeof(localizationStrings) / sizeof(CTSTR);
		Log(TEXT("Video Source Plugin strings not found, dynamically loading %d strings"), sizeof(localizationStrings) / sizeof(CTSTR));
		for (int i = 0; i < localizationStringCount; i += 2) {
			locale->AddLookupString(localizationStrings[i], localizationStrings[i + 1]);
		}
		if (!locale->HasLookup(KEY("PluginName"))) {
			AppWarning(TEXT("Uh oh..., unable to dynamically add our localization keys"));
		}
	}

	//Create mutex
	ghMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex


	

	//Register callbacks with obs
	OBSRegisterImageSourceClass(VIDEO_SOURCE_CLASS, STR("ClassName"), (OBSCREATEPROC)CreatePythonSource, (OBSCONFIGPROC)ConfigureVideoSource);


	Py_Initialize();
	PyEval_InitThreads();
	

	
	/*Must set arguments for gui to work*/
	char *argv[] = { "OBS", NULL };
	int argc = sizeof(argv) / sizeof(char*) - 1;
	PySys_SetArgv(argc, argv);
	


	//Load the OBS Extension
	initOBS();


	PyRun_SimpleString("import sys,os");
	PyRun_SimpleString("sys.path.append(\"./plugins/Python\")");
	PyRun_SimpleString("sys.stdout = open('./plugins/Python/pyOut.txt','w',0)");
	PyRun_SimpleString("sys.stderr = open('./plugins/Python/pyErr.txt','w',0)");
	
	

	/*PyRun_SimpleString("import OBS\n",
		"import sys\n"
		"from PySide import QtGui, QtCore\n"
		"import ctypes\n"
		"app = QtGui.QApplication('')\n"
	);*/
	
	
	PyThreadState *pts = PyGILState_GetThisThreadState();
	PyEval_ReleaseThread(pts);
	

	



}




void PythonPlugin::finPython(){
	
	
	if (Py_IsInitialized()){
		Py_Finalize();
	}



}
  
PythonPlugin::~PythonPlugin()
{
	/*
	DWORD dwWaitResult = WaitForSingleObject(
		ghMutex,    // handle to mutex
		INFINITE);  // no time-out interval
	*/

	PyGILState_STATE gstate;
	gstate = PyGILState_Ensure();

	Log(TEXT("Python plugin Destroyed"));
	if (isDynamicLocale) {
		int localizationStringCount = sizeof(localizationStrings) / sizeof(CTSTR);
		Log(TEXT("Video Source Plugin instance deleted; removing dynamically loaded localization strings"));
		for (int i = 0; i < localizationStringCount; i += 2) {
			locale->RemoveLookupString(localizationStrings[i]);
		}
	}

	
	
	if (Py_IsInitialized()){
		Py_Finalize();
	}

	Log(TEXT("Python plugin Destroyed"));
	isDynamicLocale = false;
	
	ReleaseMutex(ghMutex);
	CloseHandle(ghMutex);
}

CTSTR GetPluginName(){
	return STR("PluginName");
}

CTSTR GetPluginDescription()
{
	return STR("PluginDescription");
}

bool LoadPlugin(){
	if (PythonPlugin::instance != NULL)
		return false;
	PythonPlugin::instance = new PythonPlugin();
	return true;
}

void UnloadPlugin(){
	if (PythonPlugin::instance == NULL)
		return;
	delete PythonPlugin::instance;
	PythonPlugin::instance = NULL;
}


void OnStartStream()
{
}

void OnStopStream()
{
}
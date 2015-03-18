// Version 4.0
// ©2014 Starscene Software. All rights reserved. Redistribution without permission not allowed.
#pragma strict

import UnityEngine.GUILayout;
import System.Collections.Generic;
import System.IO;
import Vectrosity;

class LineMaker extends ScriptableWizard {
	var useCsharp = false;
	var objectTransform : Transform;
	var pointObjects : Transform[];
	var lines : List.<GameObject>;
	var linePoints : List.<Vector3Pair>;
	static var pointScale = .2;
	static var lineScale = .1;
	static var oldPointScale = .2;
	static var oldLineScale = .1;
	var message = "";
	var linePositions : List.<Vector3>;
	var lineRotations : List.<Quaternion>;
	var objectMaterial : Material;
	var originalMaterial : Material;
	var pointMaterial : Material;
	var lineMaterial : Material;
	var objectMesh : Mesh;
	static var objectScale : Vector3;
	var initialized = false;
	var focusTemp = false;
	var idx : int;
	var endianDiff1 : int;
	var endianDiff2 : int;
	var canUseVector2 = true;
	var floatSuffix = "";
	var newPrefix = "";
	var selectedSegments : List.<int>;
	var selectedSegmentsString : String;
	var byteBlock : byte[];
	var vectorArray : Vector3[];
	var loadedFile = false;
	
	@MenuItem ("Assets/Line Maker... %l")
	static function CreateWizard () {
		var object = Selection.activeObject as GameObject;
		if (!object) {
			EditorUtility.DisplayDialog("No object selected", "Please select an object", "Cancel");
			return;
		}
		var mf = object.GetComponentInChildren(MeshFilter) as MeshFilter;
		if (!mf) {
			EditorUtility.DisplayDialog("No MeshFilter present", "Object must have a MeshFilter component", "Cancel");
			return;
		}
		var objectMesh = mf.sharedMesh as Mesh;
		if (!objectMesh) {
			EditorUtility.DisplayDialog("No mesh present", "Object must have a mesh", "Cancel");
			return;
		}
		if (objectMesh.vertexCount > 2000) {
			EditorUtility.DisplayDialog("Too many vertices", "Please select a low-poly object", "Cancel");
			return;
		}
		objectScale = object.transform.localScale;
		objectScale = Vector3(1.0/objectScale.x, 1.0/objectScale.y, 1.0/objectScale.z);
		
		var window = ScriptableWizard.DisplayWizard("Line Maker", LineMaker);
		window.minSize = Vector2(360, 235);
	}
	
	// Initialize this way so we don't have to use static vars, so multiple wizards can be used at once with different objects
	function OnWizardUpdate () {
		if (!initialized) {
			Initialize();
		}
	}
	
	function Initialize () {
		System.Threading.Thread.CurrentThread.CurrentCulture = new System.Globalization.CultureInfo("en-US");
		var object : GameObject = (Selection.activeObject as GameObject);
		objectMesh = (object.GetComponentInChildren(MeshFilter) as MeshFilter).sharedMesh;	
		originalMaterial = object.GetComponentInChildren(Renderer).sharedMaterial;
		objectMaterial = new Material(Shader.Find("Transparent/Diffuse"));
		objectMaterial.color.a = .8;
		object.GetComponentInChildren(Renderer).material = objectMaterial;
		pointMaterial = new Material(Shader.Find("VertexLit"));
		pointMaterial.color = Color.blue;
		pointMaterial.SetColor("_Emission", Color.blue);
		pointMaterial.SetColor("_SpecColor", Color.blue);
		lineMaterial = new Material(Shader.Find("VertexLit"));
		lineMaterial.color = Color.green;
		lineMaterial.SetColor("_Emission", Color.green);
		lineMaterial.SetColor("_SpecColor", Color.green);
		
		var meshVertices = objectMesh.vertices;
		// Remove duplicate vertices
		var meshVerticesList = new List.<Vector3>(meshVertices);
		for (var i = 0; i < meshVerticesList.Count-1; i++) {
			var j = i+1;
			while (j < meshVerticesList.Count) {
				if (meshVerticesList[i] == meshVerticesList[j]) {
					meshVerticesList.RemoveAt(j);
				}
				else {
					j++;
				}
			}
		}
		meshVertices = meshVerticesList.ToArray();
		
		// See if z is substantially different on any points, in which case the option to generate a Vector2 array will not be available
		var zCoord = System.Math.Round(meshVertices[0].z, 3);
		for (i = 1; i < meshVertices.Length; i++) {
			if (!Mathf.Approximately(System.Math.Round(meshVertices[i].z, 3), zCoord)) {
				canUseVector2 = false;
				break;
			}
		}

		// Create the blue point sphere widgets
		objectTransform = object.transform;
		var objectMatrix = objectTransform.localToWorldMatrix;
		pointObjects = new Transform[meshVertices.Length];
		for (i = 0; i < pointObjects.Length; i++) {
			pointObjects[i] = GameObject.CreatePrimitive(PrimitiveType.Sphere).transform;
			pointObjects[i].position = objectMatrix.MultiplyPoint3x4(meshVertices[i]);
			pointObjects[i].parent = objectTransform;
			pointObjects[i].localScale = objectScale * pointScale;
			pointObjects[i].GetComponent(Renderer).sharedMaterial = pointMaterial;
		}
		
		lines = new List.<GameObject>();
		linePoints = new List.<Vector3Pair>();
		linePositions = new List.<Vector3>();
		lineRotations = new List.<Quaternion>();
		endianDiff1 = System.BitConverter.IsLittleEndian? 0 : 3;
		endianDiff2 = System.BitConverter.IsLittleEndian? 0 : 1;
		Selection.objects = new UnityEngine.Object[0];	// Deselect object so it's not in the way as much
		if (useCsharp) {
			floatSuffix = "f";
			newPrefix = "new ";
		}
		selectedSegmentsString = " ";
		byteBlock = new byte[4];
		initialized = true;
	}
	
	function OnFocus () {
		for (var i = 0; i < lines.Count; i++) {
			var line : GameObject = lines[i];
			// Make sure line segment is where it's supposed to be in case it was moved
			if (line) {
				line.transform.position = linePositions[i];
				line.transform.rotation = lineRotations[i];
			}
			// But if it's null, then the user must have trashed it, so delete the line
			else {
				DeleteLine(i);
			}
		}
		
		// See which line segments are selected, and make a string that lists the line segment indices
		var selectionIDs = Selection.instanceIDs;
		selectedSegments = new List.<int>();
		for (i = 0; i < lines.Count; i++) {
			for (var j = 0; j < selectionIDs.Length; j++) {
				if ((lines[i] as GameObject).GetInstanceID() == selectionIDs[j]) {
					selectedSegments.Add(i);
				}
			}
		}
		if (selectedSegments.Count > 0) {
			selectedSegmentsString = "Selected line segment " + ((selectedSegments.Count > 1)? "indices: " : "index: ");
			for (i = 0; i < selectedSegments.Count; i++) {
				selectedSegmentsString += selectedSegments[i];
				if (i < selectedSegments.Count-1) {
					selectedSegmentsString += ", ";
				}
			}
		}
		else {
			selectedSegmentsString = " ";
		}
	}
	
	function OnLostFocus () {
		selectedSegmentsString = " ";
	}
	
	function Update () {
		if (EditorApplication.isCompiling || EditorApplication.isPlaying) {
			OnDestroy();	// Otherwise, compiling scripts makes created objects not get destroyed properly
		}
		var selectionIDs = Selection.instanceIDs;
	}

	new function OnGUI () {
		BeginHorizontal();
			Label("Object: \"" + objectTransform.name + "\"");
			if (Button("Load Line File")) {
				LoadLineFile();
			}
			EndHorizontal();
		Space(5);
	
		BeginHorizontal();
			Label("Point size: ", Width(60));
			pointScale = HorizontalSlider(pointScale, .025, 1.0);
			if (pointScale != oldPointScale) {
				SetPointScale();
			}
		EndHorizontal();
		BeginHorizontal();
			Label("Line size: ", Width(60));
			lineScale = HorizontalSlider(lineScale, .0125, .5);
			if (lineScale != oldLineScale) {
				SetLineScale();
			}
		EndHorizontal();
		Space(10);
	
		BeginHorizontal();
			var buttonWidth = Screen.width/2 - 6;
			if (Button("Make Line Segment", Width(buttonWidth))) {
				message = "";
				var selectionIDs = Selection.instanceIDs;
				if (selectionIDs.Length == 2) {
					if (CheckPointID(selectionIDs[0]) && CheckPointID(selectionIDs[1])) {
						CreateLine();
					}
					else {
						message = "Must select vertex points from this object";
					}
				}
				else {
					message = "Must have two points selected";
				}
			}
			if (Button("Delete Line Segments", Width(buttonWidth))) {
				message = "";
				selectionIDs = Selection.instanceIDs;
				if (selectionIDs.Length == 0) {
					message = "Must select line segment(s) to delete";
				}
				for (var i = 0; i < selectionIDs.Length; i++) {
					var lineNumber = CheckLineID(selectionIDs[i]);
					if (lineNumber != -1) {
						DeleteLine (lineNumber);
					}
					else {
						message = "Only lines from this object can be deleted";
					}
				}
			}
		EndHorizontal();
		Space(5);

		BeginHorizontal();
			if (Button(loadedFile? "Restore Loaded Lines" : "Connect All Points", Width(buttonWidth))) {
				message = "";
				if (objectMesh.triangles.Length == 0) {
					message = "No triangles exist...must connect points manually";
				}
				else {
					DeleteAllLines();
					ConnectAllPoints();
				}
			}
			if (Button("Delete All Line Segments", Width(buttonWidth))) {
				message = "";
				DeleteAllLines();
			}
		EndHorizontal();
		Space(5);
		
		Label(selectedSegmentsString);
		Space(5);

		Label(message);
		Space(5);
		
		if (canUseVector2) {
			BeginHorizontal();
		}
		if (Button("Generate Complete Line")) {
			message = "";
			ComputeLine(true);
		}
		if (canUseVector2) {
			if (Button("Vector2", Width(100))) {
				ComputeLine(false);
			}
			EndHorizontal();
		}
		Space(2);
		
		if (canUseVector2) {
			BeginHorizontal();
		}
		if (Button("Write Complete Line to File")) {
			WriteLineToFile(true);
		}
		if (canUseVector2) {
			if (Button("Vector2", Width(100))) {
				WriteLineToFile(false);
			}
			EndHorizontal();
		}
	}
	
	function CheckPointID (thisID : int) : boolean {
		for (var i = 0; i < pointObjects.Length; i++) {
			if (pointObjects[i].gameObject.GetInstanceID() == thisID) {
				return true;
			}
		}
		return false;
	}

	function CheckLineID (thisID : int) : int {
		for (var i = 0; i < lines.Count; i++) {
			if ((lines[i] as GameObject).GetInstanceID() == thisID) {
				return i;
			}
		}
		return -1;
	}
	
	function CreateLine () {
		var selectedObjects = Selection.gameObjects;
		Line (selectedObjects[0].transform.position, selectedObjects[1].transform.position);
	}
	
	function Line (p1 : Vector3, p2 : Vector3) {
		// Make a cube midway between the two points, scaled and rotated so it connects them
		var line = GameObject.CreatePrimitive(PrimitiveType.Cube);
		line.transform.position = Vector3( (p1.x + p2.x)/2, (p1.y + p2.y)/2, (p1.z + p2.z)/2 );
		line.transform.localScale = Vector3(lineScale, lineScale, Vector3.Distance(p1, p2));
		line.transform.LookAt(p1);
		line.transform.parent = objectTransform;
		line.GetComponent(Renderer).sharedMaterial = lineMaterial;
		lines.Add(line);
		linePoints.Add(Vector3Pair(p1, p2));
		linePositions.Add(line.transform.position);
		lineRotations.Add(line.transform.rotation);
	}
	
	function DeleteLine (lineID : int) {
		message = "";
		var thisLine = lines[lineID];
		lines.RemoveAt(lineID);
		linePoints.RemoveAt(lineID);
		linePositions.RemoveAt(lineID);
		lineRotations.RemoveAt(lineID);
		if (thisLine) DestroyImmediate(thisLine);
		selectedSegmentsString = " ";
	}
	
	function ConnectAllPoints () {
		if (!loadedFile) {
			var meshTris = objectMesh.triangles;
			var meshVertices = objectMesh.vertices;
			var objectMatrix = objectTransform.localToWorldMatrix;
			var pairs = new Dictionary.<Vector3Pair, boolean>();
			
			for (var i = 0; i < meshTris.Length; i += 3) {
				var p1 = meshVertices[meshTris[i]];
				var p2 = meshVertices[meshTris[i+1]];
				CheckPoints(pairs, p1, p2, objectMatrix);
				
				p1 = meshVertices[meshTris[i+1]];
				p2 = meshVertices[meshTris[i+2]];
				CheckPoints(pairs, p1, p2, objectMatrix);
				
				p1 = meshVertices[meshTris[i+2]];
				p2 = meshVertices[meshTris[i]];
				CheckPoints(pairs, p1, p2, objectMatrix);
			}
		}
		else {
			ConnectLoadedLines();
		}
	}

	function CheckPoints (pairs : Dictionary.<Vector3Pair, boolean>, p1 : Vector3, p2 : Vector3, objectMatrix : Matrix4x4) {
		// Only add a line if the two points haven't been connected yet, so there are no duplicate lines
		var pair1 = Vector3Pair(p1, p2);
		var pair2 = Vector3Pair(p2, p1);
		if (!pairs.ContainsKey(pair1) && !pairs.ContainsKey(pair2)) {
			pairs[pair1] = true;
			pairs[pair2] = true;
			Line (objectMatrix.MultiplyPoint3x4(p1), objectMatrix.MultiplyPoint3x4(p2));
		}
	}
	
	function DeleteAllLines () {
		if (!lines) return;
		
		for (var line : GameObject in lines) {
			DestroyImmediate(line);
		}
		lines = new List.<GameObject>();
		linePoints = new List.<Vector3Pair>();
		linePositions = new List.<Vector3>();
		lineRotations = new List.<Quaternion>();
		selectedSegmentsString = " ";
	}
	
	function ComputeLine (doVector3 : boolean) {
		if (lines.Count < 1) {
			message = "No lines present";
			return;
		}

		var output = "";
		for (var i = 0; i < linePoints.Count; i++) {
			var p1 = Vector3Round(linePoints[i].p1 - objectTransform.position);
			var p2 = Vector3Round(linePoints[i].p2 - objectTransform.position);
			if (doVector3) {
				output += newPrefix + "Vector3(" + p1.x + floatSuffix + ", " + p1.y + floatSuffix + ", " + p1.z + floatSuffix + "), "
						+ newPrefix + "Vector3(" + p2.x + floatSuffix + ", " + p2.y + floatSuffix + ", " + p2.z + floatSuffix + ")";
			}
			else {
				output += newPrefix + "Vector2(" + p1.x + floatSuffix + ", " + p1.y + floatSuffix + "), "
						+ newPrefix + "Vector2(" + p2.x + floatSuffix + ", " + p2.y + floatSuffix + ")";
			}
			if (i < linePoints.Count-1) {
				output += ", ";
			}
		}
		EditorGUIUtility.systemCopyBuffer = output;
		message = "Vector line sent to copy buffer. Please paste into a script now.";
	}
	
	function Vector3Round (p : Vector3) : Vector3 {
		// Round to 3 digits after the decimal so we don't get annoying and unparseable floating point values like -5E-05
		return Vector3(System.Math.Round(p.x, 3), System.Math.Round(p.y, 3), System.Math.Round(p.z, 3));
	}
	
	function WriteLineToFile (doVector3 : boolean) {
		if (lines.Count < 1) {
			message = "No lines present";
			return;
		}

		var path = EditorUtility.SaveFilePanelInProject("Save " + (doVector3? "Vector3" : "Vector2") + " Line", objectTransform.name+"Vector.bytes", "bytes", 
														"Please enter a file name for the line data");
		if (path == "") return;

		var fileBytes = new byte[(doVector3? linePoints.Count*24 : linePoints.Count*16)];
    	idx = 0;
		for (var i = 0; i < linePoints.Count; i++) {
			var v : Vector3Pair = linePoints[i];
			var p = v.p1 - objectTransform.position;
			ConvertFloatToBytes (p.x, fileBytes);
			ConvertFloatToBytes (p.y, fileBytes);
			if (doVector3) {
				ConvertFloatToBytes (p.z, fileBytes);
			}
			p = v.p2 - objectTransform.position;
			ConvertFloatToBytes (p.x, fileBytes);
			ConvertFloatToBytes (p.y, fileBytes);
			if (doVector3) {
				ConvertFloatToBytes (p.z, fileBytes);
			}
		}

		try {
			File.WriteAllBytes(path, fileBytes);
			AssetDatabase.Refresh();
		}
		catch (err) {
			message = err.Message;
			return;
		}
		message = "File written successfully";
	}
	
	function ConvertFloatToBytes (f : float, bytes : byte[]) {
		var floatBytes = System.BitConverter.GetBytes (f);
		bytes[idx++] = floatBytes[    endianDiff1];
		bytes[idx++] = floatBytes[1 + endianDiff2];
		bytes[idx++] = floatBytes[2 - endianDiff2];
		bytes[idx++] = floatBytes[3 - endianDiff1];
	}
	
	function ConvertBytesToFloat (bytes : byte[], i : int) : float {
		byteBlock[    endianDiff1] = bytes[i  ];
		byteBlock[1 + endianDiff2] = bytes[i+1];
		byteBlock[2 - endianDiff2] = bytes[i+2];
		byteBlock[3 - endianDiff1] = bytes[i+3];
		return System.BitConverter.ToSingle (byteBlock, 0);
	}
	
	function SetPointScale () {
		oldPointScale = pointScale;
		for (po in pointObjects) {
			po.localScale = objectScale * pointScale;
		}
	}

	function SetLineScale () {
		oldLineScale = lineScale;
		for (line in lines) {
			line.transform.localScale = Vector3(lineScale*objectScale.x, lineScale*objectScale.y, line.transform.localScale.z);
		}
	}
	
	function LoadLineFile () {
		var path = EditorUtility.OpenFilePanel("Load line", Application.dataPath, "bytes");
		if (path == "") return;
		
		var bytes = File.ReadAllBytes(path);
		if ((canUseVector2 && bytes.Length % 16 != 0) || (!canUseVector2 && bytes.Length % 24 != 0)) {
			EditorUtility.DisplayDialog("Incorrect file length", "File does not seem to be a " + (canUseVector2? "2" : "3") + "D line file", "Cancel");
			return;
		}
		loadedFile = true;
		
		// Convert bytes to Vector3 array
		vectorArray = new Vector3[bytes.Length / (canUseVector2? 8 : 12)];
		var vectorType = canUseVector2? 2 : 3;
		var count = 0;
		for (var i = 0; i < bytes.Length; i += 4 * vectorType) {
			vectorArray[count++] = Vector3(ConvertBytesToFloat(bytes, i), ConvertBytesToFloat(bytes, i+4), (canUseVector2? 0 : ConvertBytesToFloat(bytes, i+8)));
		}
		
		// Remove duplicate points
		var points = new List.<Vector3>(vectorArray);
		for (i = 0; i < points.Count-1; i++) {
			for (var j = i+1; j < points.Count; j++) {
				if (points[i] == points[j]) {
					points.RemoveAt(j--);
				}
			}
		}
		
		// Create the blue point sphere widgets
		for (po in pointObjects) {
			if (po) DestroyImmediate(po.gameObject);
		}
		pointObjects = new Transform[points.Count];
		for (i = 0; i < points.Count; i++) {
			pointObjects[i] = GameObject.CreatePrimitive(PrimitiveType.Sphere).transform;
			pointObjects[i].position = points[i] + objectTransform.position;
			pointObjects[i].parent = objectTransform;
			pointObjects[i].localScale = objectScale * pointScale;
			pointObjects[i].GetComponent(Renderer).sharedMaterial = pointMaterial;
		}
		
		ConnectLoadedLines();
	}
	
	function ConnectLoadedLines () {
		for (var i = 0; i < vectorArray.Length; i += 2) {
			Line (vectorArray[i] + objectTransform.position, vectorArray[i+1] + objectTransform.position);
		}
	}

	new function OnDestroy () {
		for (po in pointObjects) {
			if (po) DestroyImmediate(po.gameObject);
		}
		DeleteAllLines();
		objectTransform.GetComponentInChildren(Renderer).material = originalMaterial;
		DestroyImmediate(objectMaterial);
		DestroyImmediate(pointMaterial);
		DestroyImmediate(lineMaterial);
	}
}
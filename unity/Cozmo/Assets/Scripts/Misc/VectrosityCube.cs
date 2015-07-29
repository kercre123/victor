using UnityEngine;
using System.Collections;
using Vectrosity;

/// <summary>
/// uses vectrosity to draw wireframe cube
/// </summary>
public class VectrosityCube : MonoBehaviour
{

	[SerializeField] Material lineMaterial;
	[SerializeField] float width = 2f;
	[SerializeField] bool debugEditing = false;

	Vector3[] cubePoints = {
		new Vector3(-0.5f, -0.5f, 0.5f),
		new Vector3(0.5f, -0.5f, 0.5f),
		new Vector3(-0.5f, 0.5f, 0.5f),
		new Vector3(-0.5f, -0.5f, 0.5f),
		new Vector3(0.5f, -0.5f, 0.5f),
		new Vector3(0.5f, 0.5f, 0.5f),
		new Vector3(0.5f, 0.5f, 0.5f),
		new Vector3(-0.5f, 0.5f, 0.5f),
		new Vector3(-0.5f, 0.5f, -0.5f),
		new Vector3(-0.5f, 0.5f, 0.5f),
		new Vector3(0.5f, 0.5f, 0.5f),
		new Vector3(0.5f, 0.5f, -0.5f),
		new Vector3(0.5f, 0.5f, -0.5f),
		new Vector3(-0.5f, 0.5f, -0.5f),
		new Vector3(-0.5f, -0.5f, -0.5f),
		new Vector3(-0.5f, 0.5f, -0.5f),
		new Vector3(0.5f, 0.5f, -0.5f),
		new Vector3(0.5f, -0.5f, -0.5f),
		new Vector3(0.5f, -0.5f, -0.5f),
		new Vector3(-0.5f, -0.5f, -0.5f),
		new Vector3(-0.5f, -0.5f, 0.5f),
		new Vector3(-0.5f, -0.5f, -0.5f),
		new Vector3(0.5f, -0.5f, -0.5f),
		new Vector3(0.5f, -0.5f, 0.5f)
	};

	VectorLine line = null;
	Color color = Color.white;

	void OnEnable()
	{
		if(!debugEditing) return;

		SetMaterial(lineMaterial);
		if(line != null) line.SetWidth(width);
		Show();
	}

	void OnDisable()
	{
		Hide();
	}

	public void Show()
	{
		if(line == null) return;
		line.active = true;
	}

	public void Hide()
	{
		if(line == null) return;
		line.active = false;
	}

	public void CreateWireFrame()
	{
		if(line != null) return;

		// Make a Vector3 array that contains points for a cube that's 1 unit in size

		// Make a line using the above points and material, with a width of 2 pixels
		line = new VectorLine("Vectrosity_" + gameObject.name, cubePoints, lineMaterial, width);
		
		// Make this transform have the vector line object that's defined above
		// This object is a rigidbody, so the vector object will do exactly what this object does
		VectorManager.ObjectSetup(gameObject, line, Visibility.Always, Brightness.None, false);

		SetColor(color);
		SetMaterial(lineMaterial);

	}

	void SetTempColor(Color color)
	{
		if(line == null) return;
		line.SetColor(color);
	}

	public void SetColor(Color color)
	{
		if(line == null) return;
		this.color = color;
		line.SetColor(color);
	}

	public void SetMaterial(Material mat)
	{
		if(line == null) return;
		lineMaterial = mat;
		line.material = mat;
	}

}

using UnityEngine;
using System.Collections;

namespace DigitalRuby.ThunderAndLightning
{
    public class DemoPathScript : MonoBehaviour
    {
        public GameObject Crate;

        private void Start()
        {
            Crate.GetComponent<Rigidbody>().angularVelocity = new Vector3(0.2f, 0.3f, 0.4f);
        }
    }
}

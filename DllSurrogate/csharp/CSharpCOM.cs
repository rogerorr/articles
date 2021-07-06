// http://www.codeproject.com/Articles/7859/Building-COM-Objects-in-C

namespace CSharp_COMObject
{
    using System.Runtime.InteropServices;

    [Guid("E7C52644-7AF1-4B8B-832C-23816F4188D9")]
    public interface CSharp_Interface
    {
        [DispId(1)]
        string GetData();
    }

    [Guid("1C5B73C2-4652-432D-AEED-3034BDB285F7"),
    ClassInterface(ClassInterfaceType.None)]
    public class CSharp_Class : CSharp_Interface
    {
        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        public static extern void OutputDebugString(string message);

        public CSharp_Class()
        {
            OutputDebugString("Created");
            System.Console.WriteLine("Created");
        }

        public string GetData()
        {
            OutputDebugString("Called");
            System.Console.WriteLine("Called");
            if (System.Environment.Is64BitProcess)
                return "Hello from 64-bit C#";
            else
                return "Hello from 32-bit C#";
        }
    }
}

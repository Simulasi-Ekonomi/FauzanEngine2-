import subprocess
import os

class AndroidBuilderSkill:
    def execute(self, command="debug"):
        script_path = os.path.join(os.path.dirname(__file__), "android_tools.sh")
        
        try:
            print(f"🚀 Memulai perintah: {command}...")
            process = subprocess.Popen(["bash", script_path, command], 
                                     stdout=subprocess.PIPE, 
                                     stderr=subprocess.STDOUT,
                                     text=True)
            
            for line in process.stdout:
                print(f"  [LOG]: {line.strip()}")
                
            process.wait()
            return "✅ Operasi Android Builder Selesai."
        except Exception as e:
            return f"❌ Terjadi kesalahan: {str(e)}"

if __name__ == "__main__":
    builder = AndroidBuilderSkill()
    print(builder.execute("debug"))

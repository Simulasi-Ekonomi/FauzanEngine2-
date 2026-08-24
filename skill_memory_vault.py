#!/usr/bin/env python3
import os
import sqlite3
import argparse

# Konfigurasi Vault Permanen Aries
ENGINE_ROOT = "/storage/emulated/0/Buku saya/FauzanEngine"
VAULT_DB = os.path.join(ENGINE_ROOT, "Aries_Subconscious_Vault.db")

def init_vault():
    conn = sqlite3.connect(VAULT_DB)
    c = conn.cursor()
    # Menggunakan FTS5: Mesin pencari internal super cepat milik SQLite
    c.execute('''CREATE VIRTUAL TABLE IF NOT EXISTS memory_vault USING fts5(
                 filename, filepath, raw_code)''')
    conn.commit()
    return conn

def assimilate_all(conn):
    print(f"[*] Mengaktifkan Hippocampus... Memindai 1745+ file ke Vault Subsadar.")
    c = conn.cursor()
    c.execute("DELETE FROM memory_vault") # Bersihkan ingatan usang
    
    count = 0
    for root, _, files in os.walk(ENGINE_ROOT):
        # Abaikan direktori yang bukan kode sumber engine
        if ".git" in root or "build" in root or "Aries_Omni_Brain" in root:
            continue
        for file in files:
            if file.endswith(('.h', '.cpp', '.hpp', '.c', '.cs', '.py', '.frag', '.vert', '.comp', '.java', '.kt', '.txt', '.md', '.sh', '.lua')):
                filepath = os.path.join(root, file)
                try:
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                        raw_code = f.read()
                    c.execute("INSERT INTO memory_vault (filename, filepath, raw_code) VALUES (?, ?, ?)",
                              (file, filepath, raw_code))
                    count += 1
                except Exception:
                    pass
    conn.commit()
    print(f"[+] Asimilasi Selesai: {count} kode sumber murni tertanam di Vault.")

def recall_memory(conn, keyword):
    c = conn.cursor()
    # Mencari kata kunci spesifik di seluruh file dalam hitungan milidetik
    c.execute("""SELECT filename, filepath FROM memory_vault 
                 WHERE memory_vault MATCH ? ORDER BY rank LIMIT 5""", (keyword,))
    results = c.fetchall()
    
    if not results:
        print(f"[-] Ingatan tentang '{keyword}' tidak ditemukan.")
        return

    print(f"[+] Ditemukan {len(results)} kaitan memori untuk '{keyword}':")
    for row in results:
        print(f" - {row[0]} (Path: {row[1]})")
    print("\n[!] Instruksi Aries: Gunakan fungsi 'read' pada path di atas untuk membuka isi kodenya.")

def read_memory(conn, filepath):
    c = conn.cursor()
    c.execute("SELECT raw_code FROM memory_vault WHERE filepath = ?", (filepath,))
    result = c.fetchone()
    if result:
        print(f"=== MEMBUKA MEMORI AKTIF: {filepath} ===")
        print(result[0])
        print("=== MEMORI DITUTUP ===")
    else:
        print("[-] File tidak ditemukan di dalam Vault.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Aries Memory Vault Skill")
    parser.add_argument("action", choices=["assimilate", "recall", "read"], help="Aksi kognitif")
    parser.add_argument("--query", type=str, default="", help="Keyword pencarian atau filepath")
    
    args = parser.parse_args()
    conn = init_vault()
    
    if args.action == "assimilate":
        assimilate_all(conn)
    elif args.action == "recall":
        if not args.query:
            print("ERROR: Butuh --query untuk recall.")
        else:
            recall_memory(conn, args.query)
    elif args.action == "read":
        if not args.query:
            print("ERROR: Butuh --query (filepath) untuk read.")
        else:
            read_memory(conn, args.query)
    
    conn.close()

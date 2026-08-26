#include "AgricultureCurriculum.h"

namespace NeoEngine {
namespace {
LessonMaterial Explanation(std::string title, std::string body, std::string source) {
    return {LessonMaterialKind::Explanation, std::move(title), std::move(body), std::move(source)};
}
LessonMaterial Rule(std::string title, std::string body, std::string source) {
    return {LessonMaterialKind::Rule, std::move(title), std::move(body), std::move(source)};
}
LessonMaterial Table(std::string title, std::string body, std::string source) {
    return {LessonMaterialKind::Table, std::move(title), std::move(body), std::move(source)};
}
LessonReward Reward(std::string flag, int64_t amount = 0) { return {std::move(flag), amount}; }
LessonCondition Condition(LessonConditionKind kind, uint64_t target) { return {kind, target}; }
}

bool BuildAgricultureCurriculum(CurriculumGraph& graph) {
    if (!graph.Initialize()) return false;
    if (!graph.AddLesson({
            "agri.orientation", "Orientasi Simulasi Agrikultur", "Memahami bahwa pemain mengelola lahan, komoditas, biaya, hasil, dan risiko dalam rantai nilai pertanian serta peternakan.", {},
            {Condition(LessonConditionKind::GrowingTilesAtLeast, 1U)},
            {Explanation("Peta sistem", "Game memodelkan keputusan dari tahap hulu sampai hilir. Nilai tambah tidak hanya berasal dari panen, tetapi juga dari pengolahan, distribusi, dan penjualan produk akhir.", "datagame.docx"),
             Explanation("Unit ekonomi", "Coin digunakan sebagai satuan ekonomi game. Keputusan produksi harus mempertimbangkan biaya, hasil, harga, BEP, dan periode produksi.", "datapeternakandanpertanian.docx")},
            {Reward("curriculum.orientation.complete")} })) return false;
    if (!graph.AddLesson({
            "agri.land-investment", "Investasi Ladang Pertanian", "Menyelesaikan dasar pengelolaan lahan pertanian berukuran small dan memahami harga pembukaan lahan berikutnya.", {"agri.orientation"},
            {Condition(LessonConditionKind::CoinsAtLeast, 100U)},
            {Table("Parameter ladang small", "Luas small 5.000 m2, kepemilikan maksimal 10 ladang. Ladang pertama gratis; ladang kedua membutuhkan 1.000.000 Coin dan harga dasar meningkat progresif 1% pada pembelian berikutnya.", "datagame.docx"),
             Rule("Batas keputusan", "Jangan membuka lahan baru sebelum modal dan proyeksi profit ladang berjalan mampu menutup harga investasi.", "datagame.docx")},
            {Reward("curriculum.land-planning.complete")} })) return false;
    if (!graph.AddLesson({
            "agri.crop-cycle", "Siklus Produksi Padi", "Mengenali hubungan antara durasi musim, biaya produksi, hasil panen, harga jual, dan titik impas padi.", {"agri.land-investment"},
            {Condition(LessonConditionKind::GameMinutesAtLeast, 2880U)},
            {Table("Parameter padi", "Siklus sekitar 4 bulan. Hasil konvensional rata-rata sekitar 5,25–6,5 ton GKP per hektare, biaya produksi sekitar Rp10.117.375–Rp22.518.314 per hektare per musim, dan BEP harga sekitar Rp3.000–Rp3.500/kg GKP.", "datapeternakandanpertanian.docx"),
             Explanation("Rantai nilai beras", "Materi game memberi contoh hasil 1.338,75 kg beras dari 0,5 ha dan menunjukkan bahwa margin dapat berpindah dari petani ke pedagang lalu restoran melalui pengolahan.", "hargakomoditi.docx")},
            {Reward("curriculum.rice-cycle.complete")} })) return false;
    if (!graph.AddLesson({
            "agri.jagung-value-chain", "Rantai Nilai Jagung", "Membaca perbedaan profit petani, grosir, dan pedagang jagung bakar dari satu hasil panen.", {"agri.crop-cycle"},
            {Condition(LessonConditionKind::HarvestedUnitsAtLeast, 2U)},
            {Table("Parameter jagung", "Produksi acuan 7–9 ton per hektare per siklus 3–4 bulan, harga petani Rp4.000–Rp5.500/kg, dan biaya produksi Rp10.000.000–Rp13.000.000 per hektare per siklus.", "datapeternakandanpertanian.docx"),
             Table("Contoh nilai tambah", "Contoh 0,5 ha menghasilkan 7.500 kg atau sekitar 26.250 tongkol. Pada simulasi contoh, profit bersih petani Rp7.600.000, grosir Rp3.000.000, dan pedagang jagung bakar Rp204.375.000.", "hargakomoditi.docx")},
            {Reward("curriculum.corn-value-chain.complete")} })) return false;
    if (!graph.AddLesson({
            "agri.broiler-production", "Produksi Ayam Pedaging", "Memahami siklus broiler, mortalitas, bobot panen, biaya pakan, dan trade-off livebird versus retail.", {"agri.orientation"},
            {Condition(LessonConditionKind::AnimalsAtLeast, 1U)},
            {Table("Parameter broiler", "Siklus sekitar 30–35 hari, mortalitas acuan 5%, bobot panen sekitar 1,5 kg per ekor, dan harga petani sekitar Rp20.000–Rp23.000/kg hidup.", "datapeternakandanpertanian.docx"),
             Explanation("Livebird dan retail", "Materi game membandingkan penjualan livebird dengan penjualan retail/pemotongan sendiri. Retail dapat menghasilkan margin lebih tinggi karena produk turunan seperti daging, ceker, kepala, dan jeroan.", "hargakomoditi.docx")},
            {Reward("curriculum.broiler-production.complete")} })) return false;
    if (!graph.AddLesson({
            "agri.price-risk", "Risiko Harga dan BEP", "Membedakan komoditas yang relatif stabil dengan komoditas volatil dan menggunakan BEP sebagai batas keputusan produksi.", {"agri.jagung-value-chain", "agri.broiler-production"},
            {Condition(LessonConditionKind::DayIndexAtLeast, 2U)},
            {Explanation("Komoditas volatil", "Cabai, tomat, dan jagung memiliki variasi harga besar; margin dapat melonjak saat langka tetapi dapat jatuh di bawah biaya saat suplai berlebih.", "datapeternakandanpertanian.docx"),
             Rule("BEP sebagai pagar", "Produksi tidak boleh dinilai hanya dari omzet. Harga jual harus dibandingkan dengan BEP, biaya tenaga kerja, input, penyusutan, dan risiko mortalitas atau penyusutan.", "datapeternakandanpertanian.docx")},
            {Reward("curriculum.price-risk.complete")} })) return false;
    if (!graph.AddLesson({
            "agri.working-capital", "Modal Kerja dan Pinjaman", "Memahami batas pinjaman berdasarkan kebutuhan maintenance, tenor game, dan nilai aset pemain.", {"agri.price-risk"},
            {Condition(LessonConditionKind::CoinsAtLeast, 100000U)},
            {Table("Pinjaman otomatis", "Pinjaman dapat dipicu ketika net cash flow negatif setelah maintenance. Tenor yang dirancang adalah 100 bulan game; kebutuhan di bawah 100.000 Coin diarahkan ke micro-loan berbasis ads.", "datagame.docx"),
             Rule("Credit tiering", "Grade 1 memakai batas maksimum 75% nilai aset untuk aset di atas 10 miliar Coin; Grade 2 memakai batas maksimum 60% nilai aset untuk aset di bawah 10 miliar Coin, dengan rentang minimum dan maksimum sesuai blueprint.", "datagame.docx")},
            {Reward("curriculum.working-capital.complete")} })) return false;
    if (!graph.AddLesson({
            "agri.enterprise-readiness", "Kesiapan Usaha Terintegrasi", "Menyelesaikan rangkaian dasar agrikultur dan membuktikan pemain mampu menjalankan state Farm tanpa mengambil alih authority ekonomi.", {"agri.working-capital"},
            {Condition(LessonConditionKind::FarmQuestCompleted, 1U)},
            {Explanation("Skala usaha", "Blueprint game menghubungkan lahan, peternakan, pertambangan, toko, payroll, maintenance, pasar, dan pemerintah AI dalam satu ekonomi yang saling memengaruhi.", "datagame.docx"),
             Explanation("Batas implementasi", "Lesson hanya membaca telemetry Farm dan snapshot waktu. FarmSystem tetap menjadi satu-satunya pemilik authority aksi, transaksi, inventory, dan state ekonomi.", "datagame.docx")},
            {Reward("curriculum.enterprise-ready.complete", 1)} })) return false;
    return graph.Finalize();
}

} // namespace NeoEngine

#include "FFExample.hpp"
extern "C" {
	#include "FFExample.h"
}

static const boost::filesystem::path TD_n = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
static const boost::filesystem::path TF_n = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();

void clean(){
	try {
		boost::filesystem::remove_all(TD_n);
		boost::filesystem::remove(TF_n);
	}
	catch (std::exception& e) {
		std::cout << e.what() << "\n";
	}
}

int main()
{
	std::atexit(clean);
	bool noInput = true;
	setlocale(LC_ALL, "ru_RU.UTF-8");
	std::ofstream TF(TF_n.c_str(), std::ofstream::out);
	if (!TF.is_open()) {
		std::cout << "Следующий файл не удалось открыть для записи: " << TF_n << "\nНажмите 'Enter' чтобы выйти ...\n";
		std::cin.get();
		return(-1);
	}
	for (auto& p : boost::filesystem::directory_iterator(boost::filesystem::current_path())) {
		if (p.path().extension() == ".mp4" && boost::regex_match(p.path().filename().string(), (boost::regex)"[0-9]{2}-[0-9]{2}-[0-9]{2}\\.mp4")) {
			if(noInput){
				noInput = false;
			}
			if(!boost::filesystem::exists(TD_n)){
				try{
					boost::filesystem::create_directory(TD_n);
				}catch (std::exception& e) {
					std::cout << e.what() << "\n";
				}
			}
            boost::filesystem::path tmp = TD_n / p.path().filename();
			TF << "file '" << tmp.c_str() << "'\n";
            if(transcode(p.path().c_str(), tmp.c_str()) == 0){
				try {
					boost::filesystem::remove(p.path());
				}
				catch (std::exception& e) {
					std::cout << e.what() << "\n";
				}
			}
		}
	}
	TF.close();
	if(!noInput){
		boost::filesystem::path tmp = boost::filesystem::current_path() / "out.mp4";
		if(concat(TF_n.c_str(), tmp.c_str()) != 0){
			std::cout << "Не удалось объеденить видео-файлы\nНажмите 'Enter' чтобы выйти ...\n";
			std::cin.get();
			return(-1);
		}
	}
	return 0;
}

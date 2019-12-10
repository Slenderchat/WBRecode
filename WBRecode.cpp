#include "WBRecode.hpp"

extern "C" {
	#include "WBRecode.h"
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

int findAndTranscode(){
	int ret = -1;
	std::ofstream TF(TF_n.c_str(), std::ofstream::out);
	if (!TF.is_open()) {
		ret = -2;
		goto ret;
	}
	for (auto& p : boost::filesystem::directory_iterator(boost::filesystem::current_path())) {
		if ((p.path().extension() == ".mp4" || p.path().extension() == ".Mp4" || p.path().extension() == ".mP4" || p.path().extension() == ".MP4") && boost::regex_match(p.path().filename().string(), (boost::regex)"[0-9]{2}-[0-9]{2}-[0-9]{2}\\.mp4")) {
            boost::filesystem::path tmp = TD_n / p.path().filename();
			TF << "file '" << tmp.c_str() << "'\n";
			ret = transcode(p.path().c_str(), tmp.c_str());
            if(ret){
				ret = -3;
				goto ret;
			}
		}
	}
	TF.close();
	ret:
	if(ret == -1){
		std::cout << "Отсутствуют входные файлы\n";
	}else if(ret == -2){
		std::cout << "Временный файл не удалось открыть для записи\n";
	}else if(ret <= -3){
		std::cout << "Ошибка при транскодировании:\n";
	}
	return(ret);
}

int main()
{
	int ret = 0;
	setlocale(LC_ALL, "ru_RU.UTF-8");
	if(!boost::filesystem::exists(TD_n)){
		try{
			boost::filesystem::create_directory(TD_n);
		}catch (std::exception& e) {
			std::cout << e.what() << "\n";
			ret = -1;
			goto ret;
		}
	}else{
		ret = -2;
		goto ret;
	}
	if(boost::filesystem::exists(TF_n)){
		ret = -3;
		goto ret;
	}
	if(!findAndTranscode()){
		boost::filesystem::path tmp = boost::filesystem::current_path() / "out.mp4";
		if(concat(TF_n.c_str(), tmp.c_str())){
			ret = -4;
			goto ret;
		}
	}else{
		ret = -5;
	}
	ret:
	if(ret){
		clean();
		if(ret == -2){
			std::cout << "Временная директория уже существует, пожалуйста попробуйте ещё раз!\n";
		}else if(ret == -3){
			std::cout << "Временный файл уже существует, пожалуйста попробуйте ещё раз!\n";
		}else if(ret == -4){
			std::cout << "Ошибка при объединении\n";
		}
		std::cout << "Нажмите 'Enter' чтобы выйти";
		std::cin.get();
	}
	return(ret);
}
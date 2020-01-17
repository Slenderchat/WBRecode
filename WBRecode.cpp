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
		std::cout << e.what() << std::endl;
	}
}

bool find(std::vector<boost::filesystem::path> *v){
	bool ret = false;
	for (auto& p : boost::filesystem::directory_iterator(boost::filesystem::current_path())) {
		if (boost::regex_match(p.path().filename().string(), (boost::regex)"[0-9]{2}-[0-9]{2}-[0-9]{2}\\.mp4")) {
            v->push_back(p.path());
			if(!ret){
				ret = true;
			}
		}
	}
	return(ret);
}

bool transcode(std::vector<boost::filesystem::path> *v){
	bool ret = true;
	std::ofstream TF(TF_n.c_str(), std::ofstream::out);
	if (!TF.is_open()) {
		ret = false;
		std::cout << "Ошибка при составлении списка файлов" << std::endl;
		return(ret);
	}
	for(int i = 0; i < v->size(); i++){
		boost::filesystem::path tmp = TD_n / v->at(i).filename();
		if(i > 0){
			std::cout << "\r";
		}
		std::cout << "Файл " << v->at(i).filename().c_str() << " [" << i + 1 << " из " << v->size() << "]" << std::flush;
		if(transcode(v->at(i).c_str(), tmp.c_str())){
			TF << "file '" << tmp.c_str() << "'" << std::endl;
		}
		else{
			ret = false;
			break;
		}
	}
	std::cout << std::endl;
	return(ret);
}

int main()
{
	std::vector<boost::filesystem::path> files;
	setlocale(LC_ALL, "ru_RU.UTF-8");
	std::cout << "Создаю временные файлы" << std::endl;
	if(!boost::filesystem::exists(TD_n)){
		try{
			boost::filesystem::create_directory(TD_n);
		}catch (std::exception& e) {
			std::cout << e.what() << std::endl;
			clean();
			return(-1);
		}
	}else{
		try{
			boost::filesystem::remove_all(TD_n);
			boost::filesystem::create_directory(TD_n);
		}catch (std::exception& e) {
			std::cout << e.what() << std::endl;
			clean();
			return(-2);
		}
	}
	if(boost::filesystem::exists(TF_n)){
		try{
			boost::filesystem::remove(TF_n);
		}catch (std::exception& e) {
			std::cout << e.what() << std::endl;
			clean();
			return(-3);
		}
	}
	std::cout << "Выполняю поиск видео-фрагментов" << std::endl;
	if(find(&files)){
		std::cout << "Начинаю процесс приведения видео-фрагментов к единому формату" << std::endl;
		if(transcode(&files)){
			boost::filesystem::path tmp = boost::filesystem::current_path() / "out.mp4";
			if(concat(TF_n.c_str(), tmp.c_str())){
				std::cout << "Всё прошло успешно!" << std::endl;
			}else{
				std::cout << "Ошибка при объединении" << std::endl;
				clean();
				return(-4);
			}
		}else{
			std::cout << "Ошибка при транскодировании" << std::endl;
			clean();
			return(-5);
		}
	}else{
		std::cout << "Видео-фрагменты в текущей папке не найдены" << std::endl;
		clean();
		return(-6);
	}
	clean();
	return(0);
}
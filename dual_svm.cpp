#include"dual_svm.h"

//----------------------------------------------------------------------------
//�����fit()�Ƴ������棬��ô��Ҫ��д���ĸ�������
void dual_svm::update_w(std::vector<double>&new_w) {
	//��Ϊ��fit����Ϊ��Ա�����ˣ����������Ȳ�д��Ҫд�Ļ�������Ҫһ��n_sample����
}
void dual_svm::update_alpha(std::vector<double>&new_alpha) {
	//��Ϊ��fit����Ϊ��Ա�����ˣ����������Ȳ�д��Ҫд�Ļ�������Ҫһ��n_feature����
}
void dual_svm::initial_w(std::vector<double>&new_w) {
	update_w(new_w);
}
void dual_svm::initial_alpha(std::vector<double>&new_alpha) {
	update_alpha(new_alpha);
}

//--------------------------------------------------------------------------------
double dual_svm::calculate_primal(Data& train_data)const {
	double P = dot_dense(w) / 2.0;// w.dot(w)
	double ave_sum = 0.0;
	if (loss == "L1_svm") {
		double temp = 0.0;
		for (int i = 0; i < train_data.n_sample; ++i) {
			temp = 1 - train_data.Y[i] * (dot_sparse(train_data, i, w));//temp = 1 - train_y(i)*train_X.row(i).dot(w);
			ave_sum += max(0.0, temp);//ave_sum += std::max(0.0, 1 - train_y(i)*train_X.row(i).dot(w))�ᱨ��,min��Ҫ��0.0�Ƚϣ���������0�Ƚ�
		}
	}
	else if (loss == "L2_svm") {
		double temp = 0.0;
		for (int i = 0; i < train_data.n_sample; ++i) {
			temp = 1 - train_data.Y[i] * (dot_sparse(train_data, i, w));//temp = 1 - train_y(i)*train_X.row(i).dot(w);//�����w.dot(train_X.row(i))��ᱨ��
			ave_sum += max(0.0, temp)*max(0.0, temp);
		}
	}//end if-loss
	P += ave_sum * C;
	return P;
}

double dual_svm::calculate_dual(Data& train_data) const {
	double D = dot_dense(w) / 2.0;//w.dot(w)
	double ave_sum = 0.0;
	if (loss == "L1_svm") {
		for (int i = 0; i < train_data.n_sample; ++i) {
			ave_sum += alpha[i];
		}
	}
	else if (loss == "L2_svm") {
		for (int i = 0; i < train_data.n_sample; ++i) {
			ave_sum += alpha[i] - alpha[i] * alpha[i] / (4.0*C);
		}
	}
	D = ave_sum - D;
	return D;
}

double dual_svm::calculate_f(Data& train_data, int n_sample, std::vector<double> &delta_w_sum, double alpha_sum, double delta_alpha_sum, double gamma, double c)const {
	double f = 0;	//��f��Ϊxk
	for (int i = 0; i < n_sample; ++i) {
		f += max(0, 1 - train_data.Y[i] * (dot_sparse(train_data, i, w) + dot_sparse(train_data, i, delta_w_sum) * gamma));
	}
	f = c * f + gamma * gamma*dot_dense(delta_w_sum) + 2 * gamma*dot_dense(w, delta_w_sum) - gamma * delta_alpha_sum + dot_dense(w) - alpha_sum;
	return f;
}

double dual_svm::calculate_nabla(Data& train_data, int n_sample, std::vector<double> delta_w_sum, double delta_alpha_sum, double gamma, double c)const {
	double nabla = 0;
	double partial = 0;	//partial�ǹ�ʽ�ĵ�һ���֣����ۼӵ���һ����
	for (int i = 0; i < n_sample; ++i) {
		if (train_data.Y[i] * dot_sparse(train_data, i, w) + train_data.Y[i] * dot_sparse(train_data, i, delta_w_sum) * gamma < 1.0) {
			partial += -1 * train_data.Y[i] * dot_sparse(train_data, i, delta_w_sum);
		}
	}
	nabla = c * partial + 2 * gamma *dot_dense(delta_w_sum) + 2 * dot_dense(delta_w_sum, w) - delta_alpha_sum;
	return nabla;
}

int dual_svm::calculate_max_w(const string filename, int n_sample, int n_feature, int block_size) {
	std::cout << "calculating max_w......" << endl;
	ifstream fin;
	fin.open(filename.c_str());

	int line = 0;		//��ǰ��������

	std::pair<int, int> *p = new pair<int, int>[n_feature];
	//std::pair<int, int> p[num_feature];
	for (int i = 0; i < n_feature; ++i) {
		p[i].first = 0;
		p[i].second = -1;
	}

	while (!fin.eof()) {
		string read_string;
		fin >> read_string;
		if (read_string == "+1" || read_string == "1") {//ÿһ�еĿ�ͷ
			++line;
		}
		else if (read_string == "-1" || read_string == "0") {
			++line;
		}
		else {//ÿһ���е������ֶ�
			int colon = read_string.find(":");
			if (colon != -1) {
				int part1 = atoi(read_string.substr(0, colon).c_str());
				int current_block_index = std::ceil(double(line) / block_size);
				if (current_block_index > p[part1].second) {
					p[part1].first++;
					p[part1].second = current_block_index;
				}
			}
		}
	}//end while(fin)
	fin.close();
	int cal_w = 0;
	for (int i = 0; i < n_feature; ++i) {
		if (p[i].first > cal_w) cal_w = p[i].first;
		//std::cout << "Column " << i << " non-zero block num: " << p[i].first << endl;
	}
	delete[]p;
	return cal_w;
}

inline void cal_beta_b(const  Data& train_data, vector<double>& deta_b, int batch_size) {
	//for mini_batch
	for (int i = 0; i < train_data.n_sample; i++) {
		int count = 0;
		for (int j = train_data.index[i]; j < train_data.index[i + 1]; j++) {
			if (train_data.X[j] != 0) {//X�ж��Ƿ���ֵ������ȥ�����if
				count++;
			}
		}
		deta_b[i] = (1 + (batch_size - 1)*(count - 1) / double(train_data.n_sample - 1));
	}
}

//****************************************************************************************liblnear�����㷨

//fit_serial����ʵ�ֵ���ʵ�Ǵ��е��㷨������liblinearʵ��,��Ӧ��SVM_DC_serial
void dual_svm::fit_serial(Data& train_data) {
	int n_sample = train_data.n_sample, n_feature = train_data.n_feature;//����ά���Ѿ���չ
	std::cout << "n_sample" << n_sample << "  n_feature" << n_feature << endl;

	//��ʼ������w,alpha
	alpha = vector<double>(n_sample, 0.0);
	w = vector<double>(n_feature, 0.0);
	double U, Dii;
	if (loss == "L2_svm") {
		U = 1e5;//������
		Dii = 0.5 / C;
	}
	else if (loss == "L1_svm") {
		U = C;
		Dii = 0.0;
	}
	else {
		std::cout << "Error! Not available loss type." << std::endl;
	}

	//normalize_data(train_data);

	std::random_device rd;
	std::default_random_engine generator(rd());
	std::uniform_int_distribution<int> distribution(0, n_sample - 1);

	for (int epoch = 0; epoch < n_epoch; ++epoch)
	{
		double dual_gap = 0.0;
		double err = 0.0;
		for (int idx = 0; idx < n_sample; ++idx)//idxֻ�ڱ��г���
		{
			int rand_id = distribution(generator);
			double g = dot_sparse(train_data, rand_id, w)*train_data.Y[rand_id] - 1 + Dii * alpha[rand_id];//(train_X.row(i).dot(w))*train_y(i) - 1 + Dii*alpha(i);
			double pg = g;
			if (std::abs(alpha[rand_id]) < tol)//alpha(rand_id)==0
			{
				pg = min(g, 0.0);
			}
			else if (std::abs(U - alpha[rand_id]) < tol)//alpha(rand_id) ==U
			{
				pg = max(g, 0.0);
			}

			if (std::abs(pg) > err)
			{
				err = std::abs(pg);//��¼ÿһ��epoch�����ֵ
			}
			if (abs(pg) > tol)
			{
				double d = min(max(alpha[rand_id] - g / (1.0 + Dii), 0.0), U) - alpha[rand_id];
				//w = w + (alpha_new - alpha(i)) * train_y(i) * train_X.row(i).transpose();
				alpha[rand_id] += d;
				//w�ĸ���
				for (int k = train_data.index[rand_id]; k < train_data.index[rand_id + 1]; ++k) {//����һ��sample rand_id
					w[train_data.col[k]] += d * train_data.Y[rand_id] * train_data.X[k];
				}
			}
		}//end for-idx

		 //calculate the dual_gap
		double Primal_val = calculate_primal(train_data);
		double Dual_val = calculate_dual(train_data);
		dual_gap = Primal_val - Dual_val;

		if (verbose) {
			Primal_val_array.push_back(Primal_val);
			Dual_val_array.push_back(Dual_val);
			dual_gap_array.push_back(dual_gap);
			cout << "epoch " << ": " << epoch
				<< " error:" << err
				<< " Primal_val: " << setiosflags(ios::fixed) << setiosflags(ios::right) << setprecision(10) << Primal_val
				<< " Dual_val: " << setiosflags(ios::fixed) << setiosflags(ios::right) << Dual_val
				<< " dual_gap:  " << setiosflags(ios::fixed) << setiosflags(ios::right) << dual_gap << endl;
		}

		if (std::abs(dual_gap) < tol)
		{
			break;
		}//end if-dual_gap
	}//end for-epoch

}

//*************************************************************************************Mini_batch
void dual_svm::fit_mini_batch(Data& train_data) {
	omp_set_num_threads(n_thread);//���ò���CPU������Ŀ
	std::cout << "n_samples" << train_data.n_sample << "n_features" << train_data.n_feature << endl;

	alpha = vector<double>(train_data.n_sample, 0);
	w = vector<double>(train_data.n_feature, 0);

	vector<double> beta_b = vector<double>(train_data.n_sample, 0);;
	cal_beta_b(train_data, beta_b, batch_size);

	double U, Dii;
	if (loss == "L2_svm") {
		U = 1e5;
		Dii = 0.5 / C;
	}
	else if (loss == "L1_svm") {
		U = C;
		Dii = 0.0;
	}
	else {
		std::cout << "Error! Not available loss type." << std::endl;
	}

	//normalize_data(train_data);

	std::random_device rd;
	std::default_random_engine generator(rd());
	std::uniform_int_distribution<int> distribution(0, train_data.n_sample - 1);
	//double err = 0.0;

	vector<int> shuffle_index(train_data.n_sample);
	for (int i = 0; i < train_data.n_sample; i++) {
		shuffle_index[i] = i;
	}

	for (int epoch = 0; epoch < n_epoch; ++epoch)
	{
		double dual_gap = 0.0;
		random_shuffle(shuffle_index.begin(), shuffle_index.end());
		for (int batch_idx = 0; batch_idx < ceil(train_data.n_sample / double(batch_size)); ++batch_idx)
		{//����һ��batch

			//--------------------------------------------------------��������ʼ
#pragma parallel for 
			for (int idx = 0; idx < batch_size; ++idx)
			{
				int i = batch_idx * batch_size + idx;//���shuffle_index�е�ȫ������
				if (i >= train_data.n_sample) {
					continue;
				}

				int rand_id = shuffle_index[i];
				double g = dot_sparse(train_data, rand_id, w)*train_data.Y[rand_id] - 1 + Dii * alpha[rand_id];
				double pg = g;
				if (std::abs(alpha[rand_id]) < tol)//alpha(i)==0
				{
					pg = min(g, 0.0);
				}
				else if (std::abs(U - alpha[rand_id]) < tol)//alpha(i) ==U
				{
					pg = max(g, 0.0);
				}

				if (abs(pg) > tol)
				{
					double d = 0.0;
					if (loss == "L1_svm") {
						d = min(max(alpha[rand_id] - g / beta_b[rand_id], 0.0), U) - alpha[rand_id];
					}
					else if (loss == "L2_svm") {
						d = min(max(alpha[rand_id] - g / (beta_b[rand_id] + 1 / (2 * C)), 0.0), U) - alpha[rand_id];
					}
					alpha[rand_id] += d;
					//w�ĸ���
					//for (int k = train_data.index[rand_id]; k<train_data.index[rand_id + 1]; ++k) {//����һ��sample i
					//	w[train_data.col[k]] += d*train_data.Y[rand_id] * train_data.X[k];
					//}
				}

			}//end for-idx
			//----------------------------------------------------------�����������
			//ע�����w�Ƿ��ڲ�����������ģ���������Ż���ĸ���
			for (int idx = 0; idx < batch_size; ++idx)
			{	//��ѵ������batchȥ����
				int i = batch_idx * batch_size + idx;
				if (i >= train_data.n_sample) {
					continue;
				}
				int rand_id = shuffle_index[i];
				for (int j = train_data.index[rand_id]; j < train_data.index[rand_id + 1]; ++j) {
					w[train_data.col[j]] += train_data.Y[rand_id] * alpha[rand_id] * train_data.X[j];
				}
			}

			//����w
			//for (int i = 0; i <train_data.n_sample; ++i)
			//{	//����������ȥ����
			//	for (int j = train_data.index[i]; j < train_data.index[i+ 1]; ++j) {
			//		//ע����������ָ��·�ʽ�������������ע�ⲻ���÷��������
			//		w[train_data.col[j]] = train_data.Y[i] * alpha[i] * train_data.X[j];
			//	}
			//}

		}
		//calculate the dual_gap
		double Primal_val = calculate_primal(train_data);
		double Dual_val = calculate_dual(train_data);
		dual_gap = Primal_val - Dual_val;

		if (verbose) {
			Primal_val_array.push_back(Primal_val);
			Dual_val_array.push_back(Dual_val);
			dual_gap_array.push_back(dual_gap);
			cout << "epoch " << ": " << epoch
				<< " Primal_val: " << setiosflags(ios::fixed) << setiosflags(ios::right) << setprecision(10) << Primal_val
				<< " Dual_val: " << setiosflags(ios::fixed) << setiosflags(ios::right) << Dual_val
				<< " dual_gap:  " << setiosflags(ios::fixed) << setiosflags(ios::right) << dual_gap << endl;
		}

		if (std::abs(dual_gap) < tol)
		{
			break;
		}//end if-dual_gap
	}//end for-epoch
}


//*************************************************************************************Passcode
void dual_svm::fit_passcode(Data& train_data)
{
	omp_set_num_threads(n_thread);//���ò���CPU������Ŀ
	alpha = vector<double>(train_data.n_sample, 0);//initialize dual variable
	w = vector<double>(train_data.n_feature, 0);
	double U, Dii;
	if (loss == "L2_svm") {
		U = 1e5;
		Dii = 0.5 / C;
	}
	else if (loss == "L1_svm") {
		U = C;
		Dii = 0.0;
	}
	else {
		std::cout << "Error! Not available loss type." << std::endl;
	}

	//normalize_data(train_data);

	std::random_device rd;
	std::default_random_engine generator(rd());
	std::uniform_int_distribution<int> distribution(0, train_data.n_sample - 1);

	for (int epoch = 0; epoch < n_epoch; ++epoch)
	{
		double dual_gap = 0.0;

#pragma omp parallel for
		for (int idx = 0; idx < train_data.n_sample; ++idx)//one epoch
		{
			int rand_id = distribution(generator);
			double g = dot_sparse(train_data, rand_id, w)*train_data.Y[rand_id] - 1 + Dii * alpha[rand_id];
			double pg = g;
			if (std::abs(alpha[rand_id]) < tol)//alpha(rand_id)==0
			{
				pg = min(g, 0.0);
			}
			else if (std::abs(U - alpha[rand_id]) < tol)//alpha(rand_id) ==U
			{
				pg = max(g, 0.0);
			}

			if (abs(pg) > tol)
			{
				double d = min(max(alpha[rand_id] - g / (1.0 + Dii), 0.0), U) - alpha[rand_id];
				alpha[rand_id] += d;
				for (int pos = train_data.index[rand_id]; pos < train_data.index[rand_id + 1]; pos++) {
					double temp = d * train_data.Y[rand_id] * train_data.X[pos];
#pragma omp atomic
					w[train_data.col[pos]] += temp;
				}
			}
		}//end for-idx

		 //calculate the dual_gap
		double Primal_val = calculate_primal(train_data);
		double Dual_val = calculate_dual(train_data);
		dual_gap = Primal_val - Dual_val;

		if (verbose) {
			Primal_val_array.push_back(Primal_val);
			Dual_val_array.push_back(Dual_val);
			dual_gap_array.push_back(dual_gap);
			cout << "epoch " << ": " << epoch
				<< " Primal_val: " << setiosflags(ios::fixed) << setiosflags(ios::right) << setprecision(10) << Primal_val
				<< " Dual_val: " << setiosflags(ios::fixed) << setiosflags(ios::right) << Dual_val
				<< " dual_gap:  " << setiosflags(ios::fixed) << setiosflags(ios::right) << dual_gap << endl;
		}

		if (std::abs(dual_gap) < tol)
		{
			break;
		}//end if-dual_gap
	}//end for-epoch
}
//***********************************************************************************************CoCoA

//���㷨ʵ�ֵ���CoCoA+,��Ӧ��SVM_DC_parallel_1
void dual_svm::fit_cocoa(Data& train_data) {
	omp_set_num_threads(n_thread);//���ò���CPU������Ŀ
	int n_sample = train_data.n_sample, n_feature = train_data.n_feature;
	std::cout << "n_sample= " << n_sample << "  n_feature= " << n_feature << endl;

	double U = 0.0;
	if (loss == "L2_svm") {
		U = (double)1e6;//Ӧ������Ϊ���ֵ
	}
	else if (loss == "L1_svm") {
		U = C;
	}
	else {
		std::cout << "Error! Not available loss type." << std::endl;
	}

	//��ʼ��w,alpha
	for (int i = 0; i < n_sample; ++i) { alpha.push_back(0.0); }
	for (int i = 0; i < n_feature; ++i) { w.push_back(0.0); }

	//normalize_data(train_data);

	std::vector<double>delta_alpha(n_sample, 0.0);

	//use to shuffle
	int block_size = std::ceil(n_sample / n_block);

	//MatrixXd delta_w(n_features, n_block);//��ʱ�洢ÿ��block�Ľ��
	std::vector<std::vector<double>> delta_w(n_block, std::vector<double>(n_feature));//δ��ʼ����

	double sigma = gamma * n_block;//CoCoA+����

	double dual_gap;
	std::chrono::system_clock::time_point t1, t2;
	for (int epoch = 0; epoch < n_epoch; ++epoch) {
		dual_gap = 0.0;
		for (int j = 0; j < delta_alpha.size(); ++j) { delta_alpha[j] = 0.0; }//delta_alpha.setZero();//����һ��epoch�ͽ���һ��reset		
		t1 = NOW;
		//---------------------------------------------------------------------------------------------------��������ʼ
#pragma omp parallel for  
		for (int block_idx = 0; block_idx < n_block; ++block_idx) {
			//ÿ��block�ȳ�ʼ��delta_w
			for (int j = 0; j < delta_w[block_idx].size(); ++j) { delta_w[block_idx][j] = 0.0; }//delta_w.setZero();

			std::random_device rd;
			std::default_random_engine generator(rd());
			std::uniform_int_distribution<int> distribution(block_idx*block_size, (block_idx + 1)*block_size - 1);

			for (int idx = 0; idx < block_size*H; ++idx)
			{//�����idx��������һ���õ�����һ���Ҫ��չ���������⾫�ȣ�������������ˡ�

				int rand_id = distribution(generator);
				//whj...
				if (block_idx == 2)
				{

					if (idx >= 1)
						idx = idx;
				}
				//...
				if (rand_id >= n_sample)
				{//ע�����������������������Ч��
					continue;
				}
				//����һ����ϵ��G��
				double temp_alpha = alpha[rand_id] + delta_alpha[rand_id];
				double G = 0.0;

				if (loss == "L1_svm") {
					G = dot_sparse(train_data, rand_id, w) + sigma * dot_sparse(train_data, rand_id, delta_w[block_idx]);
					G = G * train_data.Y[rand_id] - 1;
					//train_y(i)*train_X.row(i).dot(w + sigma*delta_w.col(block_idx)) - 1;
				}
				else if (loss == "L2_svm") {
					G = dot_sparse(train_data, rand_id, w) + sigma * dot_sparse(train_data, rand_id, delta_w[block_idx]);
					G = G * train_data.Y[rand_id] - 1 + temp_alpha / (2.0*C);
					//train_y(i)*train_X.row(i).dot(w + sigma*delta_w.col(block_idx)) - 1 + (temp_alpha) / (2.0*C);
				}

				//����PG
				double PG = G;
				if (std::abs(temp_alpha) < tol)//alpha(i) + delta_alpha(i)==0
				{
					PG = min(G, 0.0);
					//std::cout << "PG = min(G, 0.0): " << PG << endl;
				}
				else if (std::abs(U - temp_alpha) < tol)//alpha(i) + delta_alpha(i)==U
				{
					PG = max(G, 0.0);
					//std::cout << "PG = max(G, 0.0): " << PG << endl;
				}

				if (std::abs(PG) > tol)
				{
					double d = 0.0;
					if (loss == "L1_svm") {
						d = min(max(temp_alpha - G / sigma, 0.0), U) - temp_alpha;
					}
					else if (loss == "L2_svm") {
						double Denominator = sigma + 1.0 / (2.0*C);
						d = min(max(temp_alpha - G / Denominator, 0.0), U) - temp_alpha;
					}
					//���±���,������
					delta_alpha[rand_id] += d;
					//delta_w.col(block_idx) += d*train_y(i) * train_X.row(i).transpose();
					for (int k = train_data.index[rand_id]; k < train_data.index[rand_id + 1]; ++k) {
						//whj....
						if (train_data.col[k] > 300)
						{
							train_data.col[k] = train_data.col[k];
						}
						if (k > 49749)
						{
							k = k;
						}
						//debug
						//delta_w[block_idx][train_data.col[k]] += d*train_data.Y[k] * train_data.X[k];//Y[k]Խ����
						delta_w[block_idx][train_data.col[k]] += d * train_data.Y[rand_id] * train_data.X[k];
					}
				}//end if PG

			}//end for-idx
		}//end for-block_idx

		 //---------------------------------------------------------------------------------------------------�����������
		 //����alpha��w
		//alpha += delta_alpha*gamma;
		for (int j = 0; j < alpha.size(); ++j) {
			alpha[j] += gamma * delta_alpha[j];
		}
		//for (int i = 0; i<n_block; ++i)
		//{
		//	w += delta_w.col(i)*gamma;
		//}
		for (int j = 0; j < w.size(); ++j) {//ÿһ�θ���w[j]=delta_w[k][j]
			double sum = 0.0;
			for (int k = 0; k < delta_w.size(); ++k) {
				sum += delta_w[k][j];
			}
			w[j] += gamma * sum;
		}

		//������
		double Primal_val = calculate_primal(train_data);
		double Dual_val = calculate_dual(train_data);
		dual_gap = Primal_val - Dual_val;

		if (verbose) {
            Primal_val_array.push_back(Primal_val);
			Dual_val_array.push_back(Dual_val);
			dual_gap_array.push_back(dual_gap);
			cout << "epoch " << ": " << epoch << " EpochTime: " << std::chrono::duration<double>(NOW - t1).count() << "s "
				<< " Primal_val: " << setiosflags(ios::fixed) << setiosflags(ios::right) << setprecision(10) << Primal_val
				<< " Dual_val: " << setiosflags(ios::fixed) << setiosflags(ios::right) << Dual_val
				<< " dual_gap:  " << setiosflags(ios::fixed) << setiosflags(ios::right) << dual_gap << endl;
		}

		if (std::abs(dual_gap) < tol)
		{
			break;
		}
	}//end for-epoch
}//end fit


//************************************************************************************************************our parallel SDCA

void dual_svm::fit_parallel_SDCA(Data& train_data) {
	omp_set_num_threads(n_thread);
	int n_sample = train_data.n_sample, n_feature = train_data.n_feature;
	std::cout << "n_sample= " << n_sample << "  n_feature= " << n_feature << endl;//ϡ��Ȳ���
	double U;
	int block_size = std::ceil(double(n_sample) / n_block);
	//std::cout << "block_size = " << block_size << endl;
	int max_w = calculate_max_w(train_data.train_file, n_sample, n_feature, block_size);	//����pSDCA�������з���block����
	std::cout << "max_w = " << max_w << endl;
	//std::cout << "w_pi: " << w_pi << std::endl;
	int w_pi = max_w;
	double non_zero = train_data.X.size();
	double w_ave = non_zero / (double)train_data.n_feature;
	if (w_ave < max_w) {
		w_pi = w_ave + 0.4*(max_w - w_ave);
	}

	if (loss == "L2_svm") {
		U = (double)1e6;//ΪMAX
	}
	else if (loss == "L1_svm") {
		U = C;
	}
	else {
		std::cout << "Error! Not available loss type." << std::endl;
	}

	alpha = vector<double>(n_sample, 0);
	w = vector<double>(n_feature, 0);
	std::vector<double>delta_alpha(n_sample, 0.0);

	std::vector<std::vector<double> > delta_w(n_block, std::vector<double>(n_feature));//double sigma = gamma*n_block;//CoCoA+����

	double dual_gap;
	std::chrono::system_clock::time_point t1, t2;
	for (int epoch = 0; epoch < n_epoch; ++epoch) {
		dual_gap = 0.0;
		for (int j = 0; j < delta_alpha.size(); ++j) { delta_alpha[j] = 0.0; }//delta_alpha.setZero();//����һ��epoch�ͽ���һ��reset
		t1 = NOW;
		//---------------------------------------------------------------------------------------------------��������ʼ

#pragma omp parallel for  
		for (int block_idx = 0; block_idx < n_block; ++block_idx) {
			for (int j = 0; j < delta_w[block_idx].size(); ++j) { delta_w[block_idx][j] = 0.0; }
			std::random_device rd;
			std::default_random_engine generator(rd());
			std::uniform_int_distribution<int> distribution(block_idx*block_size, (block_idx + 1)*block_size - 1);

			for (int idx = 0; idx < block_size*H; ++idx)
			{//�����idx��������һ���õ�����һ���Ҫ��չ���������⾫�ȣ�������������ˡ�
				int rand_id = distribution(generator);
				if (rand_id >= n_sample)
				{//ע�����������������������Ч��
					continue;
				}
				//����һ����ϵ��G��
				double temp_alpha = alpha[rand_id] + delta_alpha[rand_id];
				double G = 0.0;

				if (loss == "L1_svm")
				{
					G = dot_sparse(train_data, rand_id, w) + w_pi * dot_sparse(train_data, rand_id, delta_w[block_idx]);
					G = G * train_data.Y[rand_id] - 1;
					//train_y(i)*train_X.row(i).dot(w + sigma*delta_w.col(block_idx)) - 1;
				}
				else if (loss == "L2_svm")
				{
					G = dot_sparse(train_data, rand_id, w) + w_pi * dot_sparse(train_data, rand_id, delta_w[block_idx]);
					G = G * train_data.Y[rand_id] - 1 + temp_alpha / (2.0*C);
					//train_y(i)*train_X.row(i).dot(w + sigma*delta_w.col(block_idx)) - 1 + (temp_alpha) / (2.0*C);
				}
				//����PG
				double PG = G;
				if (std::abs(temp_alpha) < tol)
				{//alpha(i) + delta_alpha(i)==0
					PG = min(G, 0.0);
					//std::cout << "PG = min(G, 0.0): " << PG << endl;
				}
				else if (std::abs(U - temp_alpha) < tol)
				{//alpha(i) + delta_alpha(i)==U
					PG = max(G, 0.0);
					//std::cout << "PG = max(G, 0.0): " << PG << endl;
				}

				if (std::abs(PG) > tol)
				{
					double d = 0.0;
					if (loss == "L1_svm") {
						d = min(max(temp_alpha - G / (double)w_pi, 0.0), U) - temp_alpha;
					}
					else if (loss == "L2_svm") {
						double Denominator = (double)w_pi + 1.0 / (2.0*C);
						d = min(max(temp_alpha - G / Denominator, 0.0), U) - temp_alpha;
					}
					//���±���,������
					delta_alpha[rand_id] += d;//delta_alpha(i) += d;
					for (int k = train_data.index[rand_id]; k < train_data.index[rand_id + 1]; ++k) {
						//delta_w[block_idx][train_data.col[k]] += d*train_data.Y[k] * train_data.X[k];//K����Խ��
						delta_w[block_idx][train_data.col[k]] += d * train_data.Y[rand_id] * train_data.X[k];
					}
				}//end if PG
			}//end for-idx
		}//end for-block_idx

		 //---------------------------------------------------------------------------------------------------�����������
		 //�������ŵ�gamma, ����gamma_star
		double delta_alpha_sum = 0.0;
		double alpha_sum = 0.0;

		std::vector<double> delta_w_sum(n_feature, 0.0);
		if (true) {
			//gamma = 0.5;
			delta_alpha_sum = 0.0;//ÿһ�μ��㶼Ҫ���ã��ɲ���for
			alpha_sum = 0.0;
			for (int i = 0; i < n_sample; ++i)
			{
				delta_alpha_sum += delta_alpha[i];
				alpha_sum += alpha[i];
			}
			for (int j = 0; j < n_feature; ++j) {
				delta_w_sum[j] = 0.0;//reset
				for (int k = 0; k < n_block; ++k) {
					delta_w_sum[j] += delta_w[k][j];
				}
			}
		
			std::cout << "delta_alpha_sum = " << delta_alpha_sum << endl;
			std::cout << "alpha_sum = " << alpha_sum << endl;
			std::cout << "delta_w_sum.dot(delta_w_sum) = " << dot_dense(delta_w_sum) << endl;
			std::cout << "delta_w_sum.dot(w) = " << dot_dense(delta_w_sum, w) << std::endl;

			if (true)
			{//use_best_gamma				
				if (loss == "L1_svm")
				{//�Ǿ�ȷ��������
					std::cout << "Initial gamma : " << gamma << endl;

					const double beta = 0.8;
					const double sigma = 0.01;
					const int max_loop = 30;
					double d = 0.01, gamma_next;
					cout << "+++++++++++++++++++++++++++++++++++++" << endl;
					for (double beta = 0; beta < 1; beta+=0.1) {
						//double ans = calculate_f(train_data, n_sample, delta_w_sum, alpha_sum, delta_alpha_sum, beta, C);
						//cout << "beta" << beta << " " << "ans" << ans << endl;
					}
					cout << "+++++++++++++++++++++++++++++++++++++" << endl;

					for (int loop = 0; loop < max_loop; ++loop) {
						std::cout << "loop : " << loop << endl;
						double nabla = calculate_nabla(train_data, n_sample, delta_w_sum, delta_alpha_sum, gamma, C);
						d = nabla > 0 ? -d : d;
						//double f = calculate_f(train_data, n_sample, delta_w_sum, alpha_sum, delta_alpha_sum, gamma, C);
						gamma_next = gamma + pow(beta, loop)*d;
						//double f_next = calculate_f(train_data, n_sample, delta_w_sum, alpha_sum, delta_alpha_sum, gamma_next, C);
						//std::cout << "f = " << f << endl;
						//std::cout << "f + sigma *pow(beta, loop)*nabla*d = " << f + sigma * pow(beta, loop)*nabla*d << endl;
						//std::cout << "f_next = " << f_next << endl;
						//if (f_next <= f + sigma * pow(beta, loop + 1)*nabla*d) {
						//	std::cout << "stop at loop " << loop << "   current gamma = " << gamma << endl;
						//	break;
						//}
						std::cout << "gamma : " << gamma << endl;
						std::cout << "nabla : " << nabla << endl;
						//std::cout << "f : " << f << endl;
						std::cout << endl;
					}
					gamma = gamma_next;
				}
				else if (loss == "L2_svm") {
					cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
					/*for (double beta = 0; beta < 2; beta += 0.1) {
						double ans = calculate_f(train_data, n_sample, delta_w_sum, alpha_sum, delta_alpha_sum, beta, C, delta_alpha);
						cout << "beta" << beta << " " << "ans" << ans << endl;
					}*/
					double inf = 0, sup = 1;
					for (int loop = 0; loop < n_sample; loop++) {
						double temp = train_data.Y[loop] * dot_sparse(train_data, loop, delta_w_sum);
						//cout << "temp"<<temp<<" "<< train_data.Y[loop] * dot_sparse(train_data, loop, w) << endl;
						if (temp > 0) {
							double temp_sup = (1.0 - train_data.Y[loop] * dot_sparse(train_data, loop, w)) / double(temp);
							sup = min(sup, temp_sup);
							//cout << "sup: " << sup <<" "<< temp_sup << endl;
						}
						else if (temp < 0) {
							double temp_inf = (1 - train_data.Y[loop] * dot_sparse(train_data, loop, w)) / temp;
							inf = max(inf, temp_inf);
							//cout << "inf: " << inf <<" "<< temp_inf << endl;
						}
					}
					cout <<"inf :"<< inf <<" sup: "<< sup << endl;
					if (std::abs(sup - inf) < tol) {
						gamma = (sup + inf) / 2;
					}
					else if (inf > sup) {
						double denominator = 0, numerator = 0;
						numerator = delta_alpha_sum - 2 * dot_dense(delta_w_sum, w)-0.5*dot_dense(alpha,delta_alpha)/C;
						denominator = 2 * dot_dense(delta_w_sum) + 0.5*dot_dense(delta_alpha) / C;
						if (denominator != 0) {
							gamma = numerator / denominator;
							cout << "inf > sup" << endl;
							cout << "gamma: " << gamma << endl;
						}
						else {
							cout << "inf > sup"<<endl;
							cout << "fenmu 0000000000!!!!!" << endl;
						}
					}
					else if (inf < sup) {
						double denominator = 0, numerator = 0, ans = 0, ans2 = 0;
						for (int loop = 0; loop < n_sample; loop++) {
							ans += (dot_sparse(train_data, loop, w) - train_data.Y[loop])*dot_sparse(train_data, loop, delta_w_sum);
							ans2 +=pow(dot_sparse(train_data, loop, delta_w_sum),2);
						}
						numerator = delta_alpha_sum - 2 * C*ans - 2*dot_dense(delta_w_sum, w) - 0.5*dot_dense(alpha, delta_alpha) / C;
						denominator = 2*C*ans2+2*dot_dense(delta_w_sum) + 0.5*dot_dense(delta_alpha) / C;
						if (denominator != 0) {
							gamma = numerator / denominator;
							cout << "inf < sup" << endl;
							cout << "gamma: " << gamma << endl;
						}
						else {
							//gamma = 1;
							cout << "fenmu 0000000000!!!!!" << endl;
						}
					}
				}
				//����������ûʹ��gamma_best��ȫ��ʹ��gamma���м���
				//std::cout << " gamma: " << gamma << std::endl;
				/*gamma = min(max(1.0 / n_block, gamma), 1.0);//ע��gamma����Լ���ģ��½�ʹ�û���n_block?*/
				std::cout << " gamma: " << gamma << std::endl;
				cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
			}//end if-use_best_gamma
		}

		//����alpha��w
		//alpha += delta_alpha*gamma;
		gamma = 1;
		for (int j = 0; j < alpha.size(); ++j) {
			alpha[j] += gamma * delta_alpha[j];
		}

		for (int j = 0; j < w.size(); ++j) {//ÿһ�θ���w[j]=delta_w[k][j]
			double sum = 0.0;
			for (int k = 0; k < delta_w.size(); ++k) {
				sum += delta_w[k][j];
			}
			w[j] += gamma * sum;
		}

		//������
		double Primal_val = calculate_primal(train_data);
		double Dual_val = calculate_dual(train_data);
		dual_gap = Primal_val - Dual_val;

		if (verbose) {
            Primal_val_array.push_back(Primal_val);
			Dual_val_array.push_back(Dual_val);
			dual_gap_array.push_back(dual_gap);
			cout << "epoch " << ": " << epoch << " EpochTime: " << std::chrono::duration<double>(NOW - t1).count() << "s "
				<< " Primal_val: " << setiosflags(ios::fixed) << setiosflags(ios::right) << setprecision(10) << Primal_val
				<< " Dual_val: " << setiosflags(ios::fixed) << setiosflags(ios::right) << Dual_val
				<< " dual_gap:  " << setiosflags(ios::fixed) << setiosflags(ios::right) << dual_gap << endl;
		}

		if (std::abs(dual_gap) < tol)
		{
			break;
		}
	}//end for-epoch
}//end fit




#ifndef DTREE_H
#define DTREE_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <tuple>
#include <algorithm>
#include <ctime>
#include <queue>


namespace dtree
{
	class dataset
	{
		/*
		 * ((feature_id, data), conclusion)
		 */
		struct entry
		{
			std::map<int, double> features;
			int conclusion;

			entry() : conclusion(0)
			{
			}
		};

		struct range
		{
			int min, max;

			range()
			{
				reset();
			}

			void reset()
			{
				min = std::numeric_limits<int>::max();
				max = std::numeric_limits<int>::min();
			}
		};

	private:
		std::vector<entry> _data;
		double _confusion;
		range _feature_range;
		/*
		 * Constructors
		 */
	public:
		dataset()
		{
		}

		dataset(std::ifstream& input)
		{
			std::string tmp;
			while (std::getline(input, tmp))
			{
				_data.push_back(libsvm_parser(tmp));
			}
			update_confusion();
		}

		/*
		 * Parser for LIBSVM format, and its helper functions.
		 */
	private:
		entry libsvm_parser(std::string& input)
		{
			entry new_entry;
			for (std::string element : split(input, ' '))
			{
				if (new_entry.conclusion == 0)
				{
					new_entry.conclusion = std::stoi(element);
				}
				else
				{
					auto tmp = split(element, ':');
					int feature_index = std::stoi(tmp[0]);
					new_entry.features.insert(std::make_pair(feature_index, std::stof(tmp[1])));

					if (feature_index > _feature_range.max)
					{
						_feature_range.max = feature_index;
					}
					else if (feature_index < _feature_range.min)
					{
						_feature_range.min = feature_index;
					}
				}
			}
			return new_entry;
		}

		std::vector<std::string>& split(const std::string &input, char delim, std::vector<std::string>& elements)
		{
			std::stringstream stream(input);
			std::string item;
			while (std::getline(stream, item, delim))
			{
				elements.push_back(item);
			}
			return elements;
		}

		std::vector<std::string> split(const std::string& input, char delim)
		{
			std::vector<std::string> elements;
			split(input, delim, elements);
			return elements;
		}

		/*
		 * Confusion related operations for current dataset.
		 */
	private:
		void update_confusion()
		{
			int pos_counts = 0, neg_counts = 0;
			for (const auto& entry : _data)
			{
				if (entry.conclusion > 0)
				{
					pos_counts++;
				}
				else if (entry.conclusion < 0)
				{
					neg_counts++;
				}
				else
				{
					throw std::domain_error("update_confusion(): Undefined conclusion found during the refresh.");
					std::exit(EXIT_FAILURE);
				}
			}

			_confusion = 1 - std::pow((pos_counts / (double)_data.size()), 2) - std::pow((neg_counts / (double)_data.size()), 2);
		}

		// > threshold
		double positive_confusion(int index, double threshold, int& total_counter) const
		{
			total_counter = 0;

			int pos_counts = 0, neg_counts = 0;
			for (const auto& entry : _data)
			{
				try
				{
					double value_to_test = 0;
					if (threshold == 0)
					{
						// Filter for entry with default feature
						// Perform the counting if (entry.features.count(index) != 0)
						if (entry.features.count(index) == 0)
						{
							continue;
						}
					}
					else
					{
						if (entry.features.count(index) != 0)
						{
							value_to_test = entry.features.at(index);
						}

						// Perform the counting if (value_to_test > threshold)
						if (value_to_test < threshold)
						{
							continue;
						}
					}

					total_counter++;

					if (entry.conclusion > 0)
					{
						pos_counts++;
					}
					else if (entry.conclusion < 0)
					{
						neg_counts++;
					}
					else
					{
						throw std::domain_error("positive_confusion(): Undefined conclusion found during the refresh.");
						std::exit(EXIT_FAILURE);
					}
				}
				catch (std::out_of_range e)
				{
					// oor-patch
					std::stringstream stream;
					stream << "positive_confusion(): Invalid feature index \"" << index << "\"";
					throw std::out_of_range(stream.str());
					std::exit(EXIT_FAILURE);
				}
			}

			return (1 - ((std::pow(pos_counts, 2) + std::pow(neg_counts, 2)) / std::pow((pos_counts + neg_counts), 2)));
		}

		// < threshold
		double negative_confusion(int index, double threshold, int& total_counter) const
		{
			total_counter = 0;

			int pos_counts = 0, neg_counts = 0;
			for (const auto& entry : _data)
			{
				try
				{
					if (threshold == 0)
					{
						// Filter for entry with default feature
						// Perform the counting if (entry.features.count(index) == 0)
						if (entry.features.count(index) != 0)
						{
							continue;
						}
					}
					else
					{
						double value_to_test = 0;
						if (entry.features.count(index) != 0)
						{
							value_to_test = entry.features.at(index);
						}

						// Perform the counting if (value_to_test < threshold)
						if (value_to_test > threshold)
						{
							continue;
						}
					}

					total_counter++;

					if (entry.conclusion > 0)
					{
						pos_counts++;
					}
					else if (entry.conclusion < 0)
					{
						neg_counts++;
					}
					else
					{
						throw std::domain_error("negative_confusion(): Undefined conclusion found during the refresh.");
						std::exit(EXIT_FAILURE);
					}
				}
				catch (std::out_of_range e)
				{
					std::stringstream stream;
					stream << "negative_confusion(): Invalid feature index \"" << index << "\".";
					throw std::out_of_range(stream.str());
					std::exit(EXIT_FAILURE);
				}
			}

			return (1 - ((std::pow(pos_counts, 2) + std::pow(neg_counts, 2)) / std::pow(total_counter, 2)));
		}

		/*
		 * Branching operations and corresponding support functions.
		 */
	private:
		template <typename Map>
		bool map_compare(Map const& lhs, Map const& rhs) const
		{
			// No predicate needed because there is operator== for pairs already.
			return (lhs.size() == rhs.size()) && std::equal(lhs.begin(), lhs.end(), rhs.begin());
		}

	public:
		bool can_branch() const
		{
			if (_data.size() > 1)
			{
				// Compare for maps' equality
				auto itr = _data.begin();
				for (++itr; itr != _data.end(); ++itr)
				{
					if (!map_compare(_data[0].features, itr->features))
					{
						return true;
					}
				}
				return false;
			}
			else
			{
				return false;
			}
		}

		double get_confusion() const
		{
			return _confusion;
		}

		std::pair<int, int> get_conclusion_counts() const
		{
			int pos_counts = 0, neg_counts = 0;
			for (const auto& entry : _data)
			{
				if (entry.conclusion > 0)
				{
					pos_counts++;
				}
				else if (entry.conclusion < 0)
				{
					neg_counts++;
				}
				else
				{
					throw std::domain_error("update_confusion(): Undefined conclusion found during the refresh.");
					std::exit(EXIT_FAILURE);
				}
			}

			return std::make_pair(pos_counts, neg_counts);
		}

		int get_conclusion() const
		{
			auto counts = get_conclusion_counts();

			// (pos_counts, neg_counts)
			if (std::get<0>(counts) > std::get<1>(counts))
			{
				return 1;
			}
			else if (std::get<0>(counts) < std::get<1>(counts))
			{
				return -1;
			}
			else
			{
				std::srand(std::time(0));
				return ((std::rand() % 2) ? 1 : -1);
			}
		}

		/*
		 * Separate the dataset into two according to the lowest confusion value in designated feature.
		 */
	public:
		/*
		 * Parameter: target index, separate at which threshold
		 * Return: None
		 */
		void separate(int feature_index, double threshold, dataset& pos, dataset& neg)
		{
			std::vector<entry> pos_container, neg_container;

			for (const auto& entry : _data)
			{
				try
				{
					if (threshold == 0)
					{
						// Filter for entry with default feature
						// Perform the counting if (entry.features.count(index) == 0)
						if (entry.features.count(feature_index) != 0)
						{
							pos_container.push_back(entry);
						}
						else
						{
							neg_container.push_back(entry);
						}
					}
					else
					{
						double value_to_test = 0;
						if (entry.features.count(feature_index) != 0)
						{
							value_to_test = entry.features.at(feature_index);
						}

						// Perform the counting if (value_to_test < threshold)
						if (value_to_test > threshold)
						{
							pos_container.push_back(entry);
						}
						else
						{
							neg_container.push_back(entry);
						}
					}
				}
				catch (std::out_of_range e)
				{
					throw std::out_of_range("separate(): Invalid feature index.");
					std::exit(EXIT_FAILURE);
				}
			}

			pos = pos_container;
			neg = neg_container;
		}

		/*
		 * Confusion related operations for current dataset.
		 */
	public:
		range get_feature_range() const
		{
			return _feature_range;
		}

		void update_feature_range()
		{
			_feature_range.reset();

			for (const auto& e : _data)
			{
				if (e.features.begin()->first < _feature_range.min)
				{
					_feature_range.min = e.features.begin()->first;
				}
				else if (e.features.rbegin()->first > _feature_range.max)
				{
					_feature_range.max = e.features.rbegin()->first;
				}
			}
		}

	public:
		/*
		 * Parameter: target index
		 * Return: (confusion, threshold)
		 */
		std::vector<std::pair<double, double> > get_thresholds_sequence(int feature_index)
		{
			/*
			std::set<double> values;
			for (const auto& e : _data)
			{
				try
				{
					values.insert(e.features.at(feature_index));
				}
				catch (std::out_of_range e)
				{
					// Default value as threshold.
					values.insert(0);
				}
			}
			*/

			/*
			 * (raw value, index)
			 */
			std::vector<std::pair<double, unsigned int> > values;
			for (unsigned int index = 0; index < _data.size(); index++)
			{
				try
				{
					values.push_back(std::make_pair(_data[index].features.at(feature_index), index));
				}
				catch (std::out_of_range e)
				{
					// Default value as threshold.
					values.push_back(std::make_pair(0, index));
				}
			}

			if (values.size() == 0)
			{
				throw std::runtime_error("get_thresholds_sequence(): No values in the set to build the threshold table.");
				std::exit(EXIT_FAILURE);
			}

			std::sort(values.begin(), values.end(), [](std::pair<double, int> const & t1, std::pair<double, int> const & t2)
			{
				// Compared using the raw value -> index, ascending
				if (std::get<0>(t1) == std::get<0>(t2))
				{
					return std::get<1>(t1) < std::get<1>(t2);
				}
				else
				{
					return std::get<0>(t1) < std::get<0>(t2);
				}
			});

			/*
			 * (thresholds, pos_counts, neg_counts)
			 */
			std::set<double> thresholds;
			for (auto itr = values.begin(); ;)
			{
				double a = itr->first, b;
				if (++itr == values.end())
				{
					if (a == 0)
					{
						thresholds.insert(0);
					}
					break;
				}
				else
				{
					b = itr->first;
					thresholds.insert((a + b) / 2);
				}
			}

			auto counts = get_conclusion_counts();
			int total_pos_counts = std::get<0>(counts), total_neg_counts = std::get<1>(counts);
			int current_pos_counts = 0, current_neg_counts = 0;
			int remain_pos_counts = 0, remain_neg_counts = 0;

			std::vector<std::pair<double, double> > sequence;
			unsigned int value_index;
			for (const auto& threshold : thresholds)
			{
				for (value_index = 0;(std::get<0>(values[value_index]) <= threshold) && (value_index < values.size()); value_index++)
				{
					int entry_conclusion = _data[std::get<1>(values[value_index])].conclusion;
					if (entry_conclusion > 0)
					{
						current_pos_counts++;
					}
					else if (entry_conclusion < 0)
					{
						current_neg_counts++;
					}
					else
					{
						throw std::domain_error("get_thresholds_sequence(): Undefined conclusion found during the estimation.");
						std::exit(EXIT_FAILURE);
					}
				}

				remain_pos_counts = total_pos_counts - current_pos_counts;
				remain_neg_counts = total_neg_counts - current_neg_counts;

				double pos_confusion = 1 - ((std::pow(remain_pos_counts, 2) + std::pow(remain_neg_counts, 2)) / std::pow((remain_pos_counts + remain_neg_counts), 2));
				double neg_confusion = 1 - ((std::pow(current_pos_counts, 2) + std::pow(current_neg_counts, 2)) / std::pow((current_pos_counts + current_neg_counts), 2));

				double tmp_confusion = (pos_confusion * (remain_pos_counts + remain_neg_counts) + neg_confusion * (current_pos_counts + current_neg_counts)) / _data.size();
				if (!std::isnan(tmp_confusion))
				{
					sequence.push_back(std::make_pair(tmp_confusion, threshold));
				}
			}

			/*
			std::vector<std::pair<double, double> > sequence;
			for (const auto& threshold : thresholds)
			{
				int pos_counts, neg_counts;
				double pos_confusion = positive_confusion(feature_index, threshold, pos_counts);
				double neg_confusion = negative_confusion(feature_index, threshold, neg_counts);

			#ifdef DEBUG
				std::cerr << "////////////////////" << std::endl;
				std::cerr << "pos_count=" << pos_counts << ", neg_count=" << neg_counts << std::endl;
				std::cerr << "pos_confusion=" << pos_confusion << ", neg_confusion=" << neg_confusion << std::endl;
				std::cerr << "data_size=" << _data.size() << std::endl;
				std::cerr << "////////////////////" << std::endl;
			#endif

				double tmp_confusion = (pos_confusion * pos_counts + neg_confusion * neg_counts) / _data.size();
				if (!std::isnan(tmp_confusion))
				{
					sequence.push_back(std::make_pair(tmp_confusion, threshold));
				}
			}
			*/

			return sequence;
		}

		/*
		 * Operator overloads.
		 */
	public:
		/*
		 * Output stream
		 */
		friend std::ostream& operator<<(std::ostream & stream, dataset & d)
		{
#ifdef DEBUG
			std::cerr << "confusion = " << std::fixed << std::setprecision(6) << d._confusion;
#endif

			for (auto itr = d._data.begin(); itr != d._data.end(); ++itr)
			{
				stream << std::endl;
				stream << itr->conclusion << " [ ";
				for (auto itr2 = itr->features.begin(); itr2 != itr->features.end(); ++itr2)
				{
					stream << itr2->first << '(' << itr2->second << ')' << ' ';
				}
				stream << " ]";
			}
			return stream;
		}

		/*
		 * Epsilon comparator, <=
		 */
		bool operator<=(const double & epsilon)
		{
			return _confusion <= epsilon;
		}

		/*
		 * Assignmnet operator, =
		 */
		// container(vector) to dataset
		dataset& operator=(const std::vector<entry>& rhs)
		{
			_data = rhs;
			update_confusion();
			update_feature_range();

			return *this;
		}
		// dataset to dataset
		dataset& operator=(const dataset & rhs)
		{
			_data = rhs._data;
			_confusion = rhs._confusion;
			_feature_range = rhs._feature_range;

			return *this;
		}

		/*
		 * Index operator, acquiring the conclusion for specified entry
		 */
		int operator[](const int& index) const
		{
			try
			{
				return _data.at(index).conclusion;
			}
			catch (std::exception e)
			{
				throw std::out_of_range("operator[]: Feature index out-of-bound.");
				std::exit(EXIT_FAILURE);
			}
		}
	};

	class if_tree
	{
		struct node
		{
			int feature_index, conclusion;
			double threshold;
			node* positive_child;
			node* negative_child;

			node()
				: feature_index(-1), conclusion(1), threshold(-1.0), positive_child(NULL), negative_child(NULL)
			{

			}
		};

		/*
		 * Data related private variables.
		 */
	private:
		dataset _data;
		double _epsilon;

		/*
		 * Tree related private variables.
		 */
	private:
		node* _root;

		/*
		 * Constructors and destructors
		 */
	public:
		if_tree(const dataset& data, const double& epsilon)
			: _data(data), _epsilon(epsilon)
		{
		}

		~if_tree()
		{
			destroy_tree();
		}

		/*
		 * Access variable
		 */
	public:
		void set_epsilon(const double& epsilon)
		{
			_epsilon = epsilon;
		}

		void set_dataset(const dataset& data)
		{
			_data = data;
		}

		/*
		 * Prediction function and its helper functions.
		 */
	public:
		void predict()
		{
			_root = predict(_data);

			if (_root == NULL)
			{
				throw std::runtime_error("predict(): No solution.");
				std::exit(EXIT_FAILURE);
			}
		}

	private:
		node* predict(dataset& data)
		{
			// TODO: retraverse the tree using different branching method
			if (std::isnan(data.get_confusion()))
			{
				return NULL;
			}

			node* current = new node;

			if ((data.get_confusion() <= _epsilon) || !data.can_branch())
			{
				current->conclusion = data.get_conclusion();
			}
			else
			{
				auto range = data.get_feature_range();

				/*
				 * (index, confusion, threshold)
				 */
				std::vector<std::tuple<int, double, double> > branches;

#ifdef DEBUG
				std::cerr << "********************" << std::endl;
				std::cerr << "range=[" << range.min << ", " << range.max << "]" << std::endl;
				std::cerr << std::endl;
#endif

				for (int i = range.min; i <= range.max; i++)
				{
					auto sequence = data.get_thresholds_sequence(i);

#ifdef DEBUG
					std::cerr << "i=" << i << std::endl;
#endif

					for (const auto& s : sequence)
					{
#ifdef DEBUG
						std::cerr << " -> confusion=" << std::get<0>(s) << ",\tthreshold=" << std::get<1>(s) << std::endl;
#endif

						branches.push_back(std::make_tuple(i, std::get<0>(s), std::get<1>(s)));
					}
				}

#ifdef DEBUG
				std::cerr << std::endl;
#endif

				std::sort(branches.begin(), branches.end(), [](std::tuple<int, double, double> const & t1, std::tuple<int, double, double> const & t2)
				{
					// Compared using the confusion -> threshold -> index, ascending
					if (std::get<1>(t1) == std::get<1>(t2))
					{
						if (std::get<0>(t1) == std::get<0>(t2))
						{
							return std::get<2>(t1) < std::get<2>(t2);
						}
						else
						{
							return std::get<0>(t1) < std::get<0>(t2);
						}
					}
					else
					{
						return std::get<1>(t1) < std::get<1>(t2);
					}
				});

				for (const auto& branch : branches)
				{
					current->feature_index = std::get<0>(branch);
					current->threshold = std::get<2>(branch);

#ifdef DEBUG
					std::cerr << "Separate the dataset using feature \"" << current->feature_index << "\"" << std::endl;
#endif

					dataset pos, neg;
					data.separate(current->feature_index, current->threshold, pos, neg);

					// TODO: Add back for least_confusion tracking.
#ifdef DEBUG
					std::cerr << "confusion=" << std::get<1>(branch) << " at threshold=" << current->threshold << std::endl;
					std::cerr << std::endl;
#endif

					if (std::isnan(pos.get_confusion()) || std::isnan(neg.get_confusion()))
					{
#ifdef DEBUG
						std::cerr << "...INVALID" << std::endl;
						std::cerr << "********************" << std::endl;
#endif

						continue;
					}
#ifdef DEBUG
					else
					{
						std::cerr << "Review the positive dataset" << std::endl;
						std::cerr << pos << std::endl;
						std::cerr << "Review the negative dataset" << std::endl;
						std::cerr << neg << std::endl;
						std::cerr << "********************" << std::endl;
					}
#endif

					// Check whether next value needs to be tested
					current->positive_child = predict(pos);
					if (current->positive_child == NULL)
					{
						continue;
					}
					current->negative_child = predict(neg);
					if (current->negative_child == NULL)
					{
						continue;
					}

					return current;
				}

				// If every situation in this scenario leads to partially nulled nodes, discard this result.
				delete current;
				current = NULL;
			}

			return current;
		}

		/*
		 * Tree destoryer and its helper function.
		 */
	public:
		void destroy_tree()
		{
			destroy_tree(_root);
		}

	private:
		void destroy_tree(node* leaf)
		{
			if (leaf != NULL)
			{
				destroy_tree(leaf->positive_child);
				destroy_tree(leaf->negative_child);
				delete leaf;
			}
		}

		/*
		 * Generate if-else statement.
		 */
	public:
		void generate_file(std::ostream& stream)
		{
			stream << "int tree_predict(double *attr) {" << std::endl;
			generate_file(stream, _root, 1);
			stream << '}' << std::endl;
		}

	private:
		const std::string indent_character = "  ";
		void generate_file(std::ostream& stream, node* leaf, int indent)
		{
			std::string indentations = "";
			for (int i = 0; i < indent; i++)
			{
				indentations += indent_character;
			}

			if ((leaf->positive_child == NULL) && (leaf->negative_child == NULL))
			{

				stream << indentations << "return " << leaf->conclusion << ';' << std::endl;
			}
			else if (leaf->positive_child == NULL)
			{
				throw std::runtime_error("generate_file(): Positive child is a null pointer.");
				std::exit(EXIT_FAILURE);
			}
			else if (leaf->negative_child == NULL)
			{
				throw std::runtime_error("generate_file(): Negative child is a null pointer.");
				std::exit(EXIT_FAILURE);
			}
			else
			{
				stream << indentations << "if(attr[" << leaf->feature_index << "] > " << leaf->threshold << ") {" << std::endl;
				generate_file(stream, leaf->positive_child, indent + 1);
				stream << indentations << "} else {" << std::endl;
				generate_file(stream, leaf->negative_child, indent + 1);
				stream << indentations << '}' << std::endl;
			}
		}
	};
}


#endif

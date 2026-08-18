# ifndef ENTITY_H
# define ENTITY_H

# include <string>

struct Student
{
	int         id;
	std::string name;
	std::string gender;
	long long   old_score, score;
	int         old_rank,  rank;
};

struct Rule
{
	std::string desc;
	int         delta;
};

struct Gift
{
	std::string desc;
	int         delta;
};

#endif // ENTITY_H

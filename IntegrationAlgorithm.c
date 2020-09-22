#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TreeNode {
	char *name;
	int x, y;
	int width, height;
	struct TreeNode *firstChild;
	struct TreeNode *nextSibling;
};

struct FixationNode {
	int x, y;
	int clusterid;
	struct FixationNode *next;
};

struct SegmentNode {
	char *name;
	int x, y;
	int width, height;
	struct SegmentNode *next;
};

struct TreeNode* insertTree(struct TreeNode *tree, struct TreeNode *node, char *parent_name);
void printTree(struct TreeNode *tree);

void printFixations(struct FixationNode *f);
void deleteFixations(struct FixationNode *f);

struct SegmentNode* dds_segmentation(struct TreeNode *tree, struct FixationNode* fixations, struct SegmentNode *segments, int numberofclusters, int *sizeofclusters, double confidence_level);
int checkClusters(int *clusters, int numberofclusters, int *sizeofclusters, double confidence_level);

int main() {
	double threshold;//The minimum percentage of clusters to be considered

	struct TreeNode *tree = NULL;
	struct FixationNode *fixations = NULL;
	struct SegmentNode *segments = NULL;

	FILE *f_File;
	char f_filename[81];

	int f_x, f_y;
	int f_clusterid;
	int numberofclusters;
	int *sizeofclusters;

	FILE *s_File;
	char s_filename[81];

	char s_name[81];
	int s_x, s_y;
	int s_width, s_height;
	char parent_name[81];
	
	int i;

	printf("Enter the minimum percentage of a particular cluster needed to be exist in an element (0-1): ");
	scanf("%lf", &threshold);

	printf("Enter a file name for the results of HDBSCAN: ");
	scanf("%s", f_filename);
	f_File = fopen(f_filename, "r");
	if (f_File == NULL) {
		printf("File could not be opened!\n");
		return 0;
	}

	while (fscanf(f_File, "%d%d%d", &f_x, &f_y, &f_clusterid) != EOF) {
		if (fixations == NULL) {
			fixations = (struct FixationNode *) malloc(sizeof(struct FixationNode));
			if (fixations != NULL) {
				fixations->x = f_x;
				fixations->y = f_y;
				fixations->clusterid = f_clusterid;
				fixations->next = NULL;
				numberofclusters = f_clusterid; //We aim to find the maximum cluster id as it will show the number of clusters.
			}
			else {
				printf("Memory Problem!\n");
				return 0;
			}
		}
		else {
			struct FixationNode *f_temp = fixations;
			while (f_temp->next != NULL) {
				f_temp = f_temp->next;
			}
			f_temp->next = (struct FixationNode *) malloc(sizeof(struct FixationNode));
			if (f_temp->next != NULL) {
				f_temp->next->x = f_x;
				f_temp->next->y = f_y;
				f_temp->next->clusterid = f_clusterid;
				f_temp->next->next = NULL;
				if (numberofclusters < f_clusterid)
					numberofclusters = f_clusterid;
			}
			else {
				printf("Memory Problem!\n");
				return 0;
			}
		}
	}
	fclose(f_File);
	numberofclusters += 1; // The cluster id starts with 0, so we need to add 1 to the maximum cluster id to get the number of clusters
						   //printFixations(fixations); printf("\n");
						   
	sizeofclusters = (int *) malloc(sizeof(int) * numberofclusters);
	for (i = 0; i<numberofclusters; i++) sizeofclusters[i] = 0;
	
	struct FixationNode *f_temp = fixations;
	while (f_temp != NULL) {
			++sizeofclusters[f_temp->clusterid];
			f_temp = f_temp->next;
		}

	printf("Enter a file name for the results of VIPS: ");
	scanf("%s", s_filename);
	s_File = fopen(s_filename, "r");
	if (s_File == NULL) {
		printf("File could not be opened!\n");
		return 0;
	}

	while (fscanf(s_File, "%s%d%d%d%d%s", s_name, &s_x, &s_width, &s_y, &s_height, parent_name) != EOF) {
		if (strcmp(parent_name, "-") == 0) {
			tree = (struct TreeNode *) malloc(sizeof(struct TreeNode));
			tree->name = (char *)malloc(sizeof(char) * (strlen(s_name) + 1));
			strcpy(tree->name, s_name);
			tree->x = s_x;
			tree->y = s_y;
			tree->width = s_width;
			tree->height = s_height;
			tree->firstChild = NULL;
			tree->nextSibling = NULL;
		}
		else {
			struct TreeNode * node = (struct TreeNode *) malloc(sizeof(struct TreeNode));
			node->name = (char *)malloc(sizeof(char) * (strlen(s_name) + 1));
			strcpy(node->name, s_name);
			node->x = s_x;
			node->y = s_y;
			node->width = s_width;
			node->height = s_height;
			node->firstChild = NULL;
			node->nextSibling = NULL;
			tree = insertTree(tree, node, parent_name);
		}
	}

	fclose(s_File);

	segments = dds_segmentation(tree, fixations, segments, numberofclusters, sizeofclusters, threshold);
	struct SegmentNode *s_temp = segments;
	printf("Segment                       X      Width  Y      Height\n");
	printf("----------------------------- ------ ------ ------ ------\n");
	while (s_temp != NULL) {
		printf("%-30s%-7d%-7d%-7d%-7d\n", s_temp->name, s_temp->x, s_temp->width, s_temp->y, s_temp->height);
		s_temp = s_temp->next;
	}
	
	return 0;
}

struct TreeNode* insertTree(struct TreeNode *tree, struct TreeNode *node, char *parent_name) {
	if (tree == NULL) {
		return tree;
	}
	else if (strcmp(tree->name, parent_name) == 0) {
		if (tree->firstChild == NULL) {
			tree->firstChild = node;
		}
		else {
			struct TreeNode *c_tree = tree->firstChild;
			while (c_tree->nextSibling != NULL) {
				c_tree = c_tree->nextSibling;
			}
			c_tree->nextSibling = node;
		}
		return tree;
	}
	else {
		tree->firstChild = insertTree(tree->firstChild, node, parent_name);
		tree->nextSibling = insertTree(tree->nextSibling, node, parent_name);
		return tree;
	}
}

struct SegmentNode* dds_segmentation(struct TreeNode *tree, struct FixationNode* fixations, struct SegmentNode *segments, int numberofclusters, int *sizeofclusters, double threshold) {
	if (tree == NULL) {
		return segments;
	}
	else {
		int j;
		int *clusters = (int *)malloc(numberofclusters * sizeof(int));
		for (j = 0; j<numberofclusters; j++) clusters[j] = 0;
		int counter = 0;
		int divisionneeded;
		struct FixationNode *f_temp = fixations;
		while (f_temp != NULL) {
			if (tree->x <= f_temp->x && (tree->x + tree->width) >= f_temp->x && tree->y <= f_temp->y && (tree->y + tree->height) >= f_temp->y) {
				clusters[f_temp->clusterid]++;
			}
			f_temp = f_temp->next;
		}

		divisionneeded = checkClusters(clusters, numberofclusters, sizeofclusters, threshold);

		if (divisionneeded == 0 || tree->firstChild == NULL) { //No need for futher division
			if (segments == NULL) {
				segments = (struct SegmentNode *) malloc(sizeof(struct SegmentNode));
				if (segments != NULL) {
					segments->name = (char *)malloc(sizeof(strlen(tree->name)) + 1);
					strcpy(segments->name, tree->name);
					segments->x = tree->x;
					segments->y = tree->y;
					segments->width = tree->width;
					segments->height = tree->height;
					segments->next = NULL;
				}
				else {
					printf("Memory Problem!\n");
				}
			}
			else {
				struct SegmentNode *s_temp = segments;
				while (s_temp->next != NULL) {
					s_temp = s_temp->next;
				}
				s_temp->next = (struct SegmentNode *) malloc(sizeof(struct SegmentNode));
				if (s_temp->next != NULL) {
					s_temp->next->name = (char *)malloc(sizeof(strlen(tree->name)) + 1);
					strcpy(s_temp->next->name, tree->name);
					s_temp->next->x = tree->x;
					s_temp->next->y = tree->y;
					s_temp->next->width = tree->width;
					s_temp->next->height = tree->height;
					s_temp->next->next = NULL;
				}
				else {
					printf("Memory Problem!\n");
				}
			}
			return segments;
		}
		else { //Further division is needed
			struct TreeNode *c_tree = tree->firstChild;
			while (c_tree != NULL) {
				segments = dds_segmentation(c_tree, fixations, segments, numberofclusters, sizeofclusters, threshold);
				c_tree = c_tree->nextSibling;
			}
			return segments;
		}
		free(clusters);
	}
}

int checkClusters(int *clusters, int numberofclusters, int *sizeofclusters, double threshold) {
	int maxValue = clusters[0];
	int i;
	int majorclusters = 0;
	for (i = 1; i<numberofclusters; i++) {
		if (maxValue < clusters[i])
			maxValue = clusters[i];
	}
	
	// To find major clusters based on the given confidence level
	for (i = 0; i<numberofclusters; i++) { 
		if (clusters[i] >= sizeofclusters[i] * threshold){
			majorclusters++;
		}
	}

	if (majorclusters > 1)
		return 1; // Divide!
	else
		return 0; //Do not divide!
}

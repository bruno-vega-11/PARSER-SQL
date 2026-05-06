
#pragma once
namespace SpatialIndex
{
	namespace RTree
	{
		class Leaf : public Node
		{
		public:
			~Leaf() override;

		protected:
			Leaf(RTree* pTree, id_type id);

			NodePtr chooseSubtree(const Region& mbr, uint32_t level, std::stack<id_type>& pathBuffer) override;
			NodePtr findLeaf(const Region& mbr, id_type id, std::stack<id_type>& pathBuffer) override;

			void split(uint32_t dataLength, uint8_t* pData, Region& mbr, id_type id, NodePtr& left, NodePtr& right) override;

			virtual void deleteData(const Region& mbr, id_type id, std::stack<id_type>& pathBuffer);

			friend class RTree;
			friend class BulkLoader;
		}; // Leaf
	}
}

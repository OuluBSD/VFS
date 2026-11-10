// Overlay precedence provider (API scaffold)
#ifndef _Vfs_Overlay_Precedence_h_
#define _Vfs_Overlay_Precedence_h_

// Return ordered package names or hashes, highest precedence first.
struct PackagePrecedenceProvider {
    virtual Vector<hash_t> GetPackageOrder() const = 0;
    virtual ~PackagePrecedenceProvider() {}
};

// Default implementation that returns packages in insertion order
class DefaultPrecedenceProvider : public PackagePrecedenceProvider {
    Vector<hash_t> package_order;

public:
    virtual Vector<hash_t> GetPackageOrder() const override {
        Vector<hash_t> result;
        result <<= package_order; // Use <<= operator for deep copy
        return result;
    }

    void AddPackage(hash_t pkg_hash) {
        package_order.Add(pkg_hash);
    }

    void ClearPackages() {
        package_order.Clear();
    }

    void SetPackageOrder(const Vector<hash_t>& order) {
        package_order <<= order; // Use <<= operator for deep copy
    }
};

#endif
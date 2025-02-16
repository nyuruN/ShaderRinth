#pragma once

#include <filesystem>
#include <map>
#include <memory>

#include <toml++/toml.hpp>

// Forward declares
struct Shader;
struct Texture;
struct RenderGraph;
class Geometry;

template <typename T> using AssetId = unsigned int;
template <typename T> using Assets = std::map<AssetId<T>, std::shared_ptr<T>>;

class Asset {
protected:
  std::string name;

public:
  std::string &get_name() { return name; }
};

struct AssetManager : std::enable_shared_from_this<AssetManager> {
private:
  // 0 is a reserved invalid asset id
  AssetId<Asset> next_asset_id = 1;

public:
  std::shared_ptr<Assets<Texture>> textures = std::make_shared<Assets<Texture>>();
  std::shared_ptr<Assets<Shader>> shaders = std::make_shared<Assets<Shader>>();
  std::shared_ptr<Assets<Geometry>> geometries = std::make_shared<Assets<Geometry>>();
  std::shared_ptr<Assets<RenderGraph>> graphs = std::make_shared<Assets<RenderGraph>>();

  void destroy();

  AssetId<Shader> insert_shader(std::shared_ptr<Shader> shader) {
    shaders->insert({next_asset_id++, shader});
    return next_asset_id - 1;
  }
  AssetId<Texture> insert_texture(std::shared_ptr<Texture> shader) {
    textures->insert({next_asset_id++, shader});
    return next_asset_id - 1;
  }
  AssetId<Geometry> insert_geometry(std::shared_ptr<Geometry> shader) {
    geometries->insert({next_asset_id++, shader});
    return next_asset_id - 1;
  }
  AssetId<RenderGraph> insert_graph(std::shared_ptr<RenderGraph> shader) {
    graphs->insert({next_asset_id++, shader});
    return next_asset_id - 1;
  }
  std::shared_ptr<Shader> get_shader(AssetId<Shader> &id) { return shaders->at(id); }
  std::shared_ptr<Texture> get_texture(AssetId<Texture> &id) { return textures->at(id); }
  std::shared_ptr<Geometry> get_geometry(AssetId<Geometry> &id) { return geometries->at(id); }
  std::shared_ptr<RenderGraph> get_graph(AssetId<RenderGraph> &id) { return graphs->at(id); }

  toml::table save(std::filesystem::path project_root);
  void load(toml::table &tbl, std::filesystem::path project_root);
};

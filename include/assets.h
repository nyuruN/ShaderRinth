#pragma once

#include <filesystem>
#include <map>
#include <memory>

#include <toml++/toml.hpp>

// Forward declares
class Shader;
class Texture;
class RenderGraph;
class Geometry;

template <typename T> using AssetId = unsigned int;
template <typename T> using Assets = std::map<AssetId<T>, std::shared_ptr<T>>;

class Asset {
protected:
  std::string name;

public:
  std::string &get_name() { return name; }
  virtual toml::table save(std::filesystem::path) = 0;
  /// Called on deletion
  virtual void destroy() {};
};

struct AssetManager : std::enable_shared_from_this<AssetManager> {
private:
  // 0 is a reserved invalid asset id
  AssetId<Asset> next_asset_id = 1;
  // 0 is a reserved invalid widget id
  unsigned int next_widget_id = 1;

public:
  std::shared_ptr<Assets<Texture>> mTexture = std::make_shared<Assets<Texture>>();
  std::shared_ptr<Assets<Shader>> mShader = std::make_shared<Assets<Shader>>();
  std::shared_ptr<Assets<Geometry>> mGeometry = std::make_shared<Assets<Geometry>>();
  std::shared_ptr<Assets<RenderGraph>> mRenderGraph = std::make_shared<Assets<RenderGraph>>();

  std::filesystem::path project_root;

  AssetManager() {};
  // Generates the next widget id
  unsigned int get_widget_id() { return next_widget_id++; }
  void destroy();

  AssetId<Shader> insertShader(std::shared_ptr<Shader> shader) {
    mShader->insert({next_asset_id++, shader});
    return next_asset_id - 1;
  }
  AssetId<Texture> insertTexture(std::shared_ptr<Texture> shader) {
    mTexture->insert({next_asset_id++, shader});
    return next_asset_id - 1;
  }
  AssetId<Geometry> insertGeometry(std::shared_ptr<Geometry> shader) {
    mGeometry->insert({next_asset_id++, shader});
    return next_asset_id - 1;
  }
  AssetId<RenderGraph> insertRenderGraph(std::shared_ptr<RenderGraph> shader) {
    mRenderGraph->insert({next_asset_id++, shader});
    return next_asset_id - 1;
  }
  std::optional<std::shared_ptr<Shader>> getShader(AssetId<Shader> &id) {
    try {
      return mShader->at(id);
    } catch (std::out_of_range) {
      return {};
    }
  }
  std::optional<std::shared_ptr<Texture>> getTexture(AssetId<Texture> &id) {
    try {
      return mTexture->at(id);
    } catch (std::out_of_range) {
      return {};
    }
  }
  std::optional<std::shared_ptr<Geometry>> getGeometry(AssetId<Geometry> &id) {
    try {
      return mGeometry->at(id);
    } catch (std::out_of_range) {
      return {};
    }
  }
  std::optional<std::shared_ptr<RenderGraph>> getRenderGraph(AssetId<RenderGraph> &id) {
    try {
      return mRenderGraph->at(id);
    } catch (std::out_of_range) {
      return {};
    }
  }
  std::shared_ptr<Assets<Shader>> getShaderCollection() { return mShader; }
  std::shared_ptr<Assets<Texture>> getTextureCollection() { return mTexture; }
  std::shared_ptr<Assets<Geometry>> getGeometryCollection() { return mGeometry; }
  std::shared_ptr<Assets<RenderGraph>> getRenderGraphCollection() { return mRenderGraph; }

  toml::table save(std::filesystem::path project_root);
  void load(toml::table &tbl, std::filesystem::path project_root);
};

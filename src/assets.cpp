#include "assets.h"

#include "geometry.h"
#include "graph.h"
#include "shader.h"
#include "texture.h"

void AssetManager::destroy() {
  for (auto &pair : *mTexture) {
    pair.second->destroy();
  }
  for (auto &pair : *mShader) {
    pair.second->destroy();
  }
  for (auto &pair : *mGeometry) {
    pair.second->destroy();
  }
  for (auto &pair : *mRenderGraph) {
    pair.second->destroy();
  }
  mGeometry.reset();
  mShader.reset();
  mTexture.reset();
  mRenderGraph.reset();
}
toml::table AssetManager::save(std::filesystem::path project_root) {
  this->project_root = project_root;

  toml::array tShader{};
  for (auto &pair : *mShader) {
    auto t = pair.second->save(project_root);
    t.insert("asset_id", pair.first);
    tShader.push_back(t);
  }
  toml::array tTexture{};
  for (auto &pair : *mTexture) {
    auto t = pair.second->save(project_root);
    t.insert("asset_id", pair.first);
    tTexture.push_back(t);
  }
  toml::array tGeometry{};
  for (auto &pair : *mGeometry) {
    auto t = pair.second->save(project_root);
    t.insert("asset_id", pair.first);
    tGeometry.push_back(t);
  }
  toml::array tRenderGraph{};
  for (auto &pair : *mRenderGraph) {
    auto t = pair.second->save(project_root);
    t.insert("asset_id", pair.first);
    tRenderGraph.push_back(t);
  }

  return toml::table{
      {"next_asset_id", next_asset_id},   //
      {"next_widget_id", next_widget_id}, //
      {"Shaders", tShader},
      {"Geometries", tGeometry},
      {"Textures", tTexture},
      {"Graphs", tRenderGraph},
  };
}
void AssetManager::load(toml::table &tbl, std::filesystem::path project_root) {
  this->project_root = project_root;

  next_asset_id = tbl["next_asset_id"].value<int>().value();
  next_widget_id = tbl["next_widget_id"].value<int>().value();

  for (auto &node : *tbl["Shaders"].as_array()) {
    toml::table *t = node.as_table();
    int asset_id = (*t)["asset_id"].value<int>().value();
    auto asset = Shader::load(*t, shared_from_this());
    mShader->insert(std::make_pair(asset_id, asset));
  }
  for (auto &node : *tbl["Textures"].as_array()) {
    toml::table *t = node.as_table();
    int asset_id = (*t)["asset_id"].value<int>().value();
    auto asset = Texture::load(*t, shared_from_this());
    mTexture->insert(std::make_pair(asset_id, asset));
  }
  for (auto &node : *tbl["Geometries"].as_array()) {
    toml::table *t = node.as_table();
    int asset_id = (*t)["asset_id"].value<int>().value();
    auto asset = Geometry::load(*t, shared_from_this());
    mGeometry->insert(std::make_pair(asset_id, asset));
  }
  for (auto &node : *tbl["Graphs"].as_array()) {
    toml::table *t = node.as_table();
    int asset_id = (*t)["asset_id"].value<int>().value();
    auto asset = RenderGraph::load(*t, shared_from_this());
    mRenderGraph->insert(std::make_pair(asset_id, asset));
  }
}

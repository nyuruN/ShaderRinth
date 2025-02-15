#include "assets.h"

#include "geometry.h"
#include "graph.h"
#include "shader.h"
#include "texture.h"

void AssetManager::destroy() {
  for (auto &pair : *textures) {
    pair.second->destroy();
  }
  for (auto &pair : *shaders) {
    pair.second->destroy();
  }
  geometries.reset();
  shaders.reset();
  textures.reset();
  graphs.reset();
}
toml::table AssetManager::save(std::filesystem::path project_root) {
  toml::array shaders{};
  for (auto &pair : *this->shaders) {
    auto t = pair.second->save(project_root);
    t.insert("asset_id", pair.first);
    shaders.push_back(t);
  }
  toml::array textures{};
  for (auto &pair : *this->textures) {
    auto t = pair.second->save();
    t.insert("asset_id", pair.first);
    textures.push_back(t);
  }
  toml::array geometries{};
  for (auto &pair : *this->geometries) {
    auto t = pair.second->save();
    t.insert("asset_id", pair.first);
    geometries.push_back(t);
  }
  toml::array graphs{};
  for (auto &pair : *this->graphs) {
    auto t = pair.second->save();
    t.insert("asset_id", pair.first);
    graphs.push_back(t);
  }

  return toml::table{
      {"next_shader_id", next_shader_id},
      {"next_texture_id", next_texture_id},
      {"next_geometry_id", next_geometry_id},
      {"next_graph_id", next_graph_id},
      {"Shaders", shaders},
      {"Textures", textures},
      {"Geometries", geometries},
      {"Graphs", graphs},
  };
}
void AssetManager::load(toml::table &tbl, std::filesystem::path project_root) {
  next_shader_id = tbl["next_shader_id"].value<int>().value();
  next_texture_id = tbl["next_texture_id"].value<int>().value();
  next_geometry_id = tbl["next_geometry_id"].value<int>().value();
  next_graph_id = tbl["next_graph_id"].value<int>().value();

  for (auto &node : *tbl["Shaders"].as_array()) {
    toml::table *t = node.as_table();
    int asset_id = (*t)["asset_id"].value<int>().value();
    std::string name = (*t)["name"].value<std::string>().value();
    std::string path_str = (*t)["path"].value<std::string>().value(); // Relative path

    shaders->insert({asset_id, std::make_shared<Shader>(Shader(name, project_root, path_str))});
  }
  for (auto &node : *tbl["Textures"].as_array()) {
    toml::table *t = node.as_table();
    int asset_id = (*t)["asset_id"].value<int>().value();
    std::string name = (*t)["name"].value<std::string>().value();
    std::string path_str = (*t)["path"].value<std::string>().value(); // Absolute path

    textures->insert({asset_id, std::make_shared<Texture>(Texture(name, path_str))});
  }
  for (auto &node : *tbl["Geometries"].as_array()) {
    toml::table *t = node.as_table();
    int asset_id = (*t)["asset_id"].value<int>().value();
    std::string type = (*t)["type"].value<std::string>().value();

    // We only have one geometry class for now...
    if (type == "ScreenQuadGeometry") {
      std::string name = (*t)["name"].value<std::string>().value();
      geometries->insert(
          {asset_id, std::make_shared<ScreenQuadGeometry>(ScreenQuadGeometry(name))});
    }
  }
  for (auto &node : *tbl["Graphs"].as_array()) {
    toml::table *t = node.as_table();
    int asset_id = (*t)["asset_id"].value<int>().value();

    graphs->insert(
        {asset_id, std::make_shared<RenderGraph>(RenderGraph::load(*t, shared_from_this()))});
  }
}
